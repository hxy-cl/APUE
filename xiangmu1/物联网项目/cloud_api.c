#include "iot_gateway.h"        // 包含项目公共头文件（可能包含日志、宏、传感器ID等）
#include "base64.h"             // 包含base64编码函数，用于处理API Key
#include <sys/socket.h>         // 提供socket、connect、write、close等网络函数
#include <netinet/in.h>         // 提供sockaddr_in结构体及网络字节序转换宏
#include <arpa/inet.h>          // 提供inet_aton函数，将点分十进制IP转为网络地址

/* 云平台配置常量 */
#define CLOUD_IP     "119.29.98.16"   // 云服务器IP地址
#define CLOUD_PORT   80               // 云服务器HTTP端口
#define DEVICE_ID    "5941"           // 设备ID（字符串形式）

#define SENSOR_SWITCH "16828"         // 传感器开关ID（用于ADC传感器，与config中的adc_sensor_id相同）
#define API_KEY      "eyJpYXQiOjE3NzMyODQzNzYsImV4cCI6MjA4ODY0NDM3NiwiYWxnIjoiSFMyNTYifQ.eyJpZCI6NTc3Nn0.Mz19_IBKMKlcGU4j0o-SabsyzlhzmPoG30PW5Zpky6s:"  // 云平台API Key（需要base64编码后作为Basic Auth）

/* 使用外部定义的全局变量（来自 config.c），不要重复定义 */
extern char g_cloud_api_url[256];     // 云API URL（本例未使用，但保留）
extern char g_cloud_api_token[256];   // 云API Token（未使用）
extern int g_gateway_id;              // 网关ID（未使用）
extern int g_device_id;               // 设备ID（未使用）

/* 传感器 ID 全局变量也来自外部 */
extern char g_sht30_sensor_id[32];    // SHT30传感器ID（本例未使用）
extern char g_adc_sensor_id[32];      // ADC传感器ID（未使用，但常量SENSOR_SWITCH相同）

/* 发送单个传感器数据到云平台（静态函数，仅本文件可见） */
static int upload_sensor_data(const char *sensor_id, double value)
{
    char base64_key[256] = {0};       // 存储API Key的Base64编码结果
    char post_data[128];              // 存储JSON格式的请求体，如 {"data":25.5}
    char http_request[1024];          // 存储完整的HTTP请求报文
    int sock;                         // socket文件描述符
    struct sockaddr_in server_addr;   // 服务器地址结构
    ssize_t ret;                      // 接收write和recv的返回值

    // 将API_KEY进行Base64编码，得到Basic Auth所需的凭证
    base64_encode((unsigned char *)base64_key, (const unsigned char *)API_KEY);

    // 构造JSON数据部分：{"data":数值}
    snprintf(post_data, sizeof(post_data), "{\"data\":%f}", value);

    // 构造完整的HTTP POST请求（符合HTTP/1.1标准）
    snprintf(http_request, sizeof(http_request),
        "POST /api/1.0/device/%s/sensor/%s/data HTTP/1.1\r\n"
        "Host: www.embsky.com\r\n"
        "Accept: */*\r\n"
        "Authorization: Basic %s\r\n"
        "Content-Length: %zu\r\n"
        "Content-Type: application/json;charset=utf-8\r\n"
        "Connection: close\r\n"
        "\r\n"
        "%s\r\n",
        DEVICE_ID, sensor_id, base64_key, strlen(post_data), post_data);

    // 创建TCP socket（IPv4，流式，默认协议）
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        log_write(3, "upload_sensor_data: socket creation failed"); // 错误级别3，写入日志
        return -1;
    }

    // 配置服务器地址结构
    server_addr.sin_family = AF_INET;                // IPv4地址族
    server_addr.sin_port = htons(CLOUD_PORT);        // 端口转换为网络字节序
    inet_aton(CLOUD_IP, &server_addr.sin_addr);      // 将点分十进制IP转为二进制网络地址

    // 连接到服务器
    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        log_write(3, "upload_sensor_data: connect to %s:%d failed", CLOUD_IP, CLOUD_PORT);
        close(sock);    // 关闭socket
        return -2;
    }

    // 发送HTTP请求
    ret = write(sock, http_request, strlen(http_request));
    if (ret < 0) {
        log_write(3, "upload_sensor_data: write failed");
        close(sock);
        return -3;
    }

    // 发送后，读取服务器响应（以便检查是否成功）
    char response[512] = {0};                     // 响应缓冲区
    int recv_len = recv(sock, response, sizeof(response) - 1, 0); // 接收数据，留1字节给'\0'
    if (recv_len > 0) {
        response[recv_len] = '\0';                // 字符串结束符
        // 简单检查响应中是否包含 "200 OK" 或业务成功字段 "\"code\":200"
        if (strstr(response, "200 OK") == NULL && strstr(response, "\"code\":200") == NULL) {
            log_write(3, "HTTP response error: %.200s", response);
            close(sock);
            return -4;
        }
    } else {
        log_write(3, "No response from cloud");
        close(sock);
        return -5;
    }

    close(sock);    // 关闭socket
    return 0;       // 上传成功

    // 注意：下面两行是多余的重复代码（编译时会警告，但不影响运行）
    close(sock);
    return 0;
}

/* 云API模块初始化函数 */
int cloud_api_init(void)
{
    log_write(1, "Cloud API initialized (socket + Basic Auth)"); // 级别1，信息日志
    return 0;    // 总是成功
}

/* 云API模块清理函数（释放资源） */
void cloud_api_cleanup(void)
{
    log_write(1, "Cloud API cleaned up");
    // 本实现无动态资源，仅记录日志
}

/* 对外接口：上传传感器数据到云平台（带重试机制） */
void cloud_api_upload(const char *sensor_id, const char *value)
{
    double numeric_value;           // 转换后的数值
    int retry = 3;                  // 重试次数（最多3次）
    int ret;                        // 上传函数的返回值

    // 提取数值（支持 "40/30" 格式？但这里 sensor_id 已经对应单一值，value 应为纯数字）
    numeric_value = atof(value);    // 尝试转换为浮点数
    if (numeric_value == 0 && value[0] != '0') { // 如果转换结果为0但字符串不是"0"，可能包含分隔符
        const char *slash = strchr(value, '/');  // 查找斜杠'/'
        if (slash && *(slash + 1)) {             // 如果斜杠存在且后面有字符
            numeric_value = atof(slash + 1);     // 取斜杠后面的部分（兼容意外传入"40/30"时取后半部分）
        }
    }
    
    // 循环重试上传
    while (retry--) {
        ret = upload_sensor_data(sensor_id, numeric_value); // 调用底层上传函数
        if (ret == 0) {                                     // 成功
            log_write(1, "Upload success: sensor=%s, value=%.2f", sensor_id, numeric_value);
            return;                                         // 直接返回
        }
        log_write(2, "Upload failed, retries left: %d", retry); // 级别2，警告信息
        usleep(100000); // 等待100ms再重试
    }
    // 所有重试都失败
    log_write(3, "Upload failed after retries: sensor=%s, value=%s", sensor_id, value);
}
