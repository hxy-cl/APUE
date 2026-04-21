#include "iot_gateway.h"        // 项目公共头文件（全局声明、宏定义等）
#include "pool.h"               // 线程池模块接口
#include "tbf.h"                // 令牌桶限流器接口
#include "process_pool.h"       // 进程池模块接口
#include "ipc_modules.h"        // IPC模块接口（消息队列、共享内存等）
#include "alarm_scheduler.h"    // 闹钟调度器接口
#include "io_multiplexing.h"    // IO多路复用主循环接口

// 全局变量定义（在头文件中声明为 extern，这里定义）
volatile sig_atomic_t g_shutdown = 0;   // 原子退出标志，收到退出信号时置1
int g_listen_fd = -1;                   // TCP监听套接字文件描述符
int g_udp_fd = -1;                      // UDP套接字文件描述符
int g_signal_pipe[2];                   // 信号管道（写端[1]用于信号处理，读端[0]用于poll）
pool_t *g_thread_pool = NULL;           // 全局线程池指针

// 删除 static 关键字，使函数可被其他模块调用（原注释提示）
// 初始化TCP服务器：创建socket，绑定端口，开始监听，设置非阻塞，加入poll
// 参数: 无
// 返回值: 无
void tcp_server_init(void) {
    // 创建TCP socket（IPv4，流式）
    g_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    int opt = 1;
    // 设置SO_REUSEADDR，允许端口重用，避免“Address already in use”
    setsockopt(g_listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;              // IPv4
    addr.sin_addr.s_addr = INADDR_ANY;      // 监听所有网络接口
    addr.sin_port = htons(TCP_PORT);        // 端口号（宏定义在protocol.h中）
    bind(g_listen_fd, (struct sockaddr*)&addr, sizeof(addr)); // 绑定地址
    listen(g_listen_fd, 128);               // 开始监听，最大待处理连接数128
    fcntl(g_listen_fd, F_SETFL, O_NONBLOCK); // 设置为非阻塞模式
    poll_add_fd(g_listen_fd, POLLIN);       // 将监听套接字加入poll，关注可读事件
}

// 初始化UDP服务器：创建socket，绑定端口，设置非阻塞，加入poll
// 参数: 无
// 返回值: 无
void udp_server_init(void) {
    g_udp_fd = socket(AF_INET, SOCK_DGRAM, 0);   // 创建UDP socket
    if (g_udp_fd < 0) {
        log_write(3, "UDP socket creation failed: %s", strerror(errno));
        return;
    }
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));          // 清空地址结构
    addr.sin_family = AF_INET;               // IPv4
    addr.sin_addr.s_addr = INADDR_ANY;       // 监听所有接口
    addr.sin_port = htons(UDP_PORT);         // UDP端口号（宏定义）
    if (bind(g_udp_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        log_write(3, "UDP bind port %d failed: %s", UDP_PORT, strerror(errno));
        close(g_udp_fd);
        g_udp_fd = -1;
        return;
    }
    fcntl(g_udp_fd, F_SETFL, O_NONBLOCK);    // 设置为非阻塞
    poll_add_fd(g_udp_fd, POLLIN);           // 加入poll，关注可读
    log_write(1, "UDP server initialized on port %d", UDP_PORT);
}

// 心跳检测回调函数（由闹钟调度器定期调用）
// 参数: arg - 未使用（保留）
// 返回值: 无
static void heartbeat_check(void *arg) {
    (void)arg;  // 消除未使用参数警告
    log_write(1, "Heartbeat check");   // 记录心跳日志（实际可在此处理超时连接等）
}

// 优雅退出信号处理函数（由信号触发，最终调用）
// 参数: signo - 信号编号（未使用，但保留接口）
// 返回值: 无
void graceful_shutdown(int signo) {
    (void)signo; // 消除警告
    log_write(1, "Shutting down...");
    // 依次销毁各模块资源
    pool_destroy(g_thread_pool);            // 销毁线程池（等待任务完成，释放资源）
    process_pool_destroy();                 // 销毁进程池（终止子进程，清理管道）
    alarm_scheduler_destroy();              // 销毁闹钟调度器（恢复定时器，释放节点）
    tbf_destroy_all();                      // 销毁所有令牌桶实例
    cloud_api_cleanup();                    // 清理云API模块（释放资源）
    close(g_listen_fd);                     // 关闭TCP监听套接字
    close(g_udp_fd);                        // 关闭UDP套接字
    pidfile_remove();                       // 删除PID文件
    exit(0);                                // 退出进程
}

// 内部信号处理函数（将信号编号写入信号管道，以转换为IO事件）
// 参数: sig - 收到的信号编号
// 返回值: 无
static void signal_handler(int sig) {
    // 将信号值（一个字节）写入管道写端，忽略返回值（避免编译警告）
    if(write(g_signal_pipe[1], &sig, 1)<0){};
}

// 初始化信号管道：创建管道，设置读端非阻塞，加入poll监听
// 参数: 无
// 返回值: 无
static void signal_pipe_init(void) {
    if(pipe(g_signal_pipe)<0){};   // 创建匿名管道，忽略错误
    fcntl(g_signal_pipe[0], F_SETFL, O_NONBLOCK); // 读端非阻塞
    poll_add_fd(g_signal_pipe[0], POLLIN);        // 加入poll，关注可读
}

// 初始化信号处理：创建信号管道，注册信号处理函数
// 参数: 无
// 返回值: 无
void signal_init(void) {
    signal_pipe_init();                      // 先创建管道
    signal(SIGTERM, signal_handler);         // 捕获终止信号（kill默认）
    signal(SIGINT, signal_handler);          // 捕获中断信号（Ctrl+C）
    signal(SIGHUP, signal_handler);          // 捕获挂起信号（用于配置重载）
    signal(SIGPIPE, SIG_IGN);                // 忽略SIGPIPE，防止写已关闭socket时崩溃
}

// 主函数：程序入口
// 参数: argc - 命令行参数个数
//       argv - 参数数组，argv[1]为配置文件路径，argv[2]为"-d"表示守护进程化
// 返回值: 0表示正常退出
int main(int argc, char *argv[]) {
    int daemon_flag = 0;                     // 守护进程标志，0=前台，1=后台
    // 检查命令行参数：如果参数数量大于2且第二个参数是"-d"，则设置守护标志
    if (argc > 2 && strcmp(argv[2], "-d") == 0) daemon_flag = 1;
    // 加载配置文件：若未提供配置文件路径则使用默认 CONFIG_PATH
    config_load(argc > 1 ? argv[1] : CONFIG_PATH);
    log_init(LOG_DIR);                       // 初始化日志系统，目录为 ./log
    if (daemon_flag) daemonize();            // 如果需要，将进程转为守护进程
    pidfile_create();                        // 创建PID文件并加锁，防止多实例
    signal_init();                           // 初始化信号处理
    tcp_server_init();                       // 启动TCP服务器
    udp_server_init();                       // 启动UDP服务器
    pool_init(&g_thread_pool, THREAD_POOL_SIZE); // 创建线程池（大小为宏定义）
    process_pool_init(PROCESS_POOL_SIZE, NULL);  // 创建进程池（处理函数暂时为空）
    alarm_scheduler_init();                  // 初始化闹钟调度器（setitimer）
    // 添加一个周期性的心跳检测任务（间隔 HEARTBEAT_INTERVAL 秒）
    alarm_add_task(HEARTBEAT_INTERVAL, heartbeat_check, NULL);
    if (cloud_api_init() != 0) {             // 初始化云API模块
        log_write(3, "Cloud API init failed");
    }
    poll_loop();                             // 进入主事件循环（永不返回，直到收到退出信号）
    graceful_shutdown(0);                    // 理论上不会执行到这里，但保留
    return 0;
}
