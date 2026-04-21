/* poll主循环，集成TCP/UDP、信号管道 */
#include "iot_gateway.h"         // 项目公共头文件（包含日志宏、全局变量声明等）
#include "io_multiplexing.h"     // 本模块的接口声明
#include "pool.h"                // 线程池相关（本例未直接使用，但可能后续扩展）
#include "tbf.h"                 // 令牌桶限流器接口
#include <string.h>              // 字符串操作函数（strlen, strncmp, strtok等）
#include <ctype.h>               // 字符类型判断（本例未直接使用）

#define MAX_EVENTS 10000         // 最多监听的文件描述符数量（实际动态扩展）

static struct pollfd *pfds;      // poll 文件描述符数组（动态分配）
static int nfds;                 // 当前数组中的有效元素个数
static int tbf_id;               // 令牌桶实例ID（用于UDP限流）
extern pool_t *g_thread_pool;    // 外部定义的线程池指针（本例未使用，预留）

/* 引用 config.c 中定义的传感器ID全局变量 */
extern char g_sht11_humi_id[32];   // SHT11湿度传感器ID（实际发送到云平台）
extern char g_sht11_temp_id[32];   // SHT11温度传感器ID
extern char g_adc_sensor_id[32];   // ADC光敏传感器ID

/* 解析并上报 STM32 格式的数据
 * 格式示例: "STM32: SHT30=30.59/46.61, ADC=512, DHT11=25/60, TIME=12345"
 * 函数功能: 提取 SHT30 的温度/湿度 和 ADC 的值，通过 cloud_api_upload 上报到云平台
 * 参数: buf - 接收到的UDP数据包内容（以'\0'结尾的字符串）
 * 返回值: 无
 */
static void parse_and_upload_stm32(const char *buf)
{
    const char *p = buf;   // 当前解析位置指针
    // 检查是否以 "STM32:" 开头（不区分大小写？这里要求精确匹配）
    if (strncmp(p, "STM32:", 6) != 0) {
        log_write(2, "Not STM32 format, skip: %s", buf);  // 警告级别日志，跳过
        return;
    }
    p += 6;   // 跳过 "STM32:" 前缀，指向后面的字段串

    // 创建工作缓冲区，因为 strtok 会修改原字符串
    char work_buf[512];
    strncpy(work_buf, p, sizeof(work_buf) - 1);
    work_buf[sizeof(work_buf) - 1] = '\0';   // 确保以'\0'结尾

    // 按逗号分割各个字段，例如 "SHT30=30.59/46.61", "ADC=512", ...
    char *token = strtok(work_buf, ",");
    while (token != NULL) {
        // 去除键和值前面的空格
        while (*token == ' ') token++;
        // 查找等号分隔符
        char *eq = strchr(token, '=');
        if (eq == NULL) {
            token = strtok(NULL, ",");   // 无效字段，跳过
            continue;
        }
        *eq = '\0';               // 将等号替换为字符串结束符，分开 key 和 value
        char *key = token;        // key 指向字段名
        char *value = eq + 1;     // value 指向等号后的内容

        // 去除 key 首尾的空格/制表符
        while (*key == ' ') key++;
        char *key_end = key + strlen(key) - 1;
        while (key_end > key && (*key_end == ' ' || *key_end == '\t')) {
            *key_end = '\0';
            key_end--;
        }
        // 去除 value 首尾的空格/制表符
        while (*value == ' ') value++;
        char *val_end = value + strlen(value) - 1;
        while (val_end > value && (*val_end == ' ' || *val_end == '\t')) {
            *val_end = '\0';
            val_end--;
        }

        // 只处理 SHT30 和 ADC，忽略 DHT11 和 TIME 等其它字段
        if (strcmp(key, "SHT30") == 0) {
            char temp_str[16] = {0};   // 温度字符串
            char humi_str[16] = {0};   // 湿度字符串
            // 格式: "温度/湿度"，例如 "30.59/46.61"，尝试解析
            if (sscanf(value, "%15[^/]/%15s", temp_str, humi_str) == 2) {
                // 上报温度到云平台（使用温度传感器ID）
                cloud_api_upload(g_sht11_temp_id, temp_str);   // 温度 -> 16817
                // 上报湿度到云平台（使用湿度传感器ID）
                cloud_api_upload(g_sht11_humi_id, humi_str);   // 湿度 -> 16816
                log_write(1, "Uploaded SHT30 temp=%s humi=%s", temp_str, humi_str);
            } else {
                log_write(2, "Invalid SHT30 value format: %s", value);
            }
        }
        else if (strcmp(key, "ADC") == 0) {
            // 直接上报ADC数值（光敏传感器）
            cloud_api_upload(g_adc_sensor_id, value);
            log_write(1, "Uploaded ADC: %s", value);
        }
        // 忽略 DHT11, TIME 等字段（不做处理）

        token = strtok(NULL, ",");   // 继续下一个字段
    }
}

/* 处理 TCP 监听套接字上的新连接请求
 * 函数功能: accept 新连接，设置为非阻塞，并添加到 poll 监听集合中
 * 参数: 无（使用全局 g_listen_fd）
 * 返回值: 无
 */
static void handle_tcp_accept(void)
{
    struct sockaddr_in cli;          // 客户端地址结构
    socklen_t len = sizeof(cli);     // 地址长度
    // 接受新连接（非阻塞模式，若无可立即接受的连接则返回 -1）
    int fd = accept(g_listen_fd, (struct sockaddr*)&cli, &len);
    if (fd < 0) return;              // 忽略错误（例如 EAGAIN）
    // 设置新连接为非阻塞模式
    fcntl(fd, F_SETFL, O_NONBLOCK);
    // 将该文件描述符加入 poll 监听，关注可读事件
    poll_add_fd(fd, POLLIN);
}

/* 处理已连接的 TCP 客户端的数据可读事件
 * 函数功能: 读取数据，并原样回显（简单 Echo 服务）
 * 参数: fd - 客户端 socket 文件描述符
 * 返回值: 无
 */
static void handle_tcp_read(int fd)
{
    char buf[4096];                  // 接收缓冲区
    int n = read(fd, buf, sizeof(buf) - 1);  // 非阻塞读取，留1字节给'\0'
    if (n <= 0) {                    // 连接关闭或错误
        close(fd);                   // 关闭 socket
        poll_del_fd(fd);             // 从 poll 集合中移除
        return;
    }
    buf[n] = '\0';                   // 字符串结束符（便于日志）
    // 回显 "ECHO: " + 原始数据
    if (write(fd, "ECHO: ", 6) < 0) {}   // 忽略写错误（可能连接已断开）
    if (write(fd, buf, n) < 0) {}        // 回显用户数据
}

/* 处理 UDP 套接字上的数据可读事件
 * 函数功能: 循环读取所有待处理的 UDP 数据报，进行令牌桶限流，解析并上报
 * 参数: 无（使用全局 g_udp_fd 和 tbf_id）
 * 返回值: 无
 */
static void handle_udp_read(void)
{
    char buf[2048];                      // UDP 数据报接收缓冲区
    struct sockaddr_in src;              // 发送方地址
    socklen_t len = sizeof(src);         // 地址长度
    int n;                               // 接收到的字节数

    // 循环读取，直到没有更多数据（非阻塞模式下 recvfrom 返回 EAGAIN/EWOULDBLOCK）
    while (1) {
        len = sizeof(src);
        n = recvfrom(g_udp_fd, buf, sizeof(buf) - 1, 0,
                     (struct sockaddr*)&src, &len);
        if (n <= 0) {   // 出错或无数据
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                log_write(3, "UDP recvfrom error: %s", strerror(errno));
            }
            break;  // 没有更多数据，退出循环
        }

        // 令牌桶限流：每个 UDP 包消耗 1 个令牌
        if (tbf_fetch_token(tbf_id, 1) == 1) {   // 成功获取令牌
            buf[n] = '\0';   // 字符串结束
            log_write(1, "UDP recv from %s: %s", inet_ntoa(src.sin_addr), buf);

            // 检查是否为 STM32 格式数据
            if (strncmp(buf, "STM32:", 6) == 0) {
                parse_and_upload_stm32(buf);   // 解析并上报传感器数据
            } else {
                log_write(2, "Unsupported UDP format (not STM32): %s", buf);
            }
        } else {
            log_write(2, "UDP rate limit exceeded, dropping packet");
            // 令牌不足时丢弃当前包，继续处理下一个（不阻塞）
        }
    }
}

/* 处理信号管道中的信号
 * 函数功能: 从管道读取信号值，根据信号类型设置全局标志或执行热加载
 * 参数: 无（使用全局 g_signal_pipe[0] 和 g_shutdown）
 * 返回值: 无
 */
static void handle_signal_pipe(void)
{
    char sig;   // 接收信号编号
    // 从管道读取一个字节（信号值）
    if (read(g_signal_pipe[0], &sig, 1) < 0) {};  // 忽略读取错误
    if (sig == SIGTERM || sig == SIGINT) {
        g_shutdown = 1;          // 设置全局退出标志，主循环将结束
    } else if (sig == SIGHUP) {
        config_load(CONFIG_PATH);   // 重新加载配置文件
        log_write(1, "Config reloaded");
    }
}

/* 向 poll 监听集合中添加一个文件描述符
 * 参数: fd - 要添加的文件描述符
 *       events - 感兴趣的事件（如 POLLIN）
 * 返回值: 无
 */
void poll_add_fd(int fd, int events)
{
    // 先尝试在现有数组中找一个空闲位置（fd == -1 表示空闲）
    for (int i = 0; i < nfds; i++) {
        if (pfds[i].fd == -1) {
            pfds[i].fd = fd;
            pfds[i].events = events;
            return;
        }
    }
    // 没有空闲位置，动态扩展数组
    pfds = realloc(pfds, (nfds + 1) * sizeof(struct pollfd));
    pfds[nfds].fd = fd;
    pfds[nfds].events = events;
    nfds++;
}

/* 从 poll 监听集合中删除一个文件描述符并关闭它
 * 参数: fd - 要删除的文件描述符
 * 返回值: 无
 */
void poll_del_fd(int fd)
{
    for (int i = 0; i < nfds; i++) {
        if (pfds[i].fd == fd) {
            pfds[i].fd = -1;      // 标记为无效，不立即压缩数组
            close(fd);            // 关闭文件描述符
            break;
        }
    }
}

/* 主事件循环（永不返回，直到 g_shutdown 为真）
 * 函数功能: 初始化令牌桶，循环调用 poll 等待事件，并分发处理
 * 参数: 无
 * 返回值: 无
 */
void poll_loop(void)
{
    // 初始化 UDP 限流令牌桶：每秒 CPS 个令牌，容量 BURST
    tbf_id = tbf_init(TBF_CPS, TBF_BURST);
    while (!g_shutdown) {   // 全局退出标志，由信号处理或其它逻辑设置
        // 调用 poll，超时 1000 毫秒（1秒），便于定期检查退出标志
        int ret = poll(pfds, nfds, 1000);
        if (ret < 0) continue;   // 被信号中断，继续循环
        // 遍历所有文件描述符，处理就绪事件
        for (int i = 0; i < nfds; i++) {
            if (pfds[i].fd == -1) continue;        // 跳过无效条目
            if (pfds[i].revents & POLLIN) {        // 可读事件
                if (pfds[i].fd == g_listen_fd) {
                    handle_tcp_accept();            // TCP 监听套接字 -> 接受新连接
                } else if (pfds[i].fd == g_udp_fd) {
                    handle_udp_read();              // UDP 套接字 -> 接收数据报
                } else if (pfds[i].fd == g_signal_pipe[0]) {
                    handle_signal_pipe();           // 信号管道 -> 处理信号
                } else {
                    handle_tcp_read(pfds[i].fd);    // 已连接的 TCP 客户端 -> 读取数据
                }
            }
        }
    }
}
