#ifndef __IOT_GATEWAY_H            // 头文件保护宏：防止重复包含
#define __IOT_GATEWAY_H

// 标准C库头文件
#include <stdio.h>                 // 标准输入输出（printf, perror等）
#include <stdlib.h>                // 标准库函数（exit, malloc, atoi等）
#include <string.h>                // 字符串操作（strcpy, strlen等）
#include <unistd.h>                // POSIX系统调用（read, write, close等）
#include <fcntl.h>                 // 文件控制（open, fcntl等）
#include <errno.h>                 // 错误码定义（errno变量）
#include <signal.h>                // 信号处理（signal, sigaction等）
#include <time.h>                  // 时间函数（time, localtime等）

// Linux系统编程头文件
#include <sys/types.h>             // 系统数据类型（pid_t, size_t等）
#include <sys/socket.h>            // socket编程接口
#include <sys/stat.h>              // 文件状态（mkdir, chmod等）
#include <sys/ipc.h>               // System V IPC 键值生成
#include <sys/msg.h>               // System V 消息队列
#include <sys/shm.h>               // System V 共享内存
#include <sys/sem.h>               // System V 信号量
#include <netinet/in.h>            // 网络地址结构（sockaddr_in）
#include <arpa/inet.h>             // 网络地址转换函数（inet_aton等）
#include <poll.h>                  // poll IO多路复用
#include <pthread.h>               // 线程库

#include "protocol.h"              // 新增：引入端口及心跳间隔定义（自定义协议头）

/* 配置文件路径 */
#define CONFIG_PATH     "./iot_gateway.conf"   // 配置文件路径（相对路径）
#define PID_FILE        "./iot_gateway.pid"    // PID文件路径（用于防止多实例）
#define LOG_DIR         "./log"                // 日志文件存储目录

/* 最大连接数 */
#define MAX_CONNECTIONS 5000        // 支持的最大TCP连接数（用于预分配数组）

/* 令牌桶参数 */
#define TBF_CPS         1000         // 令牌桶每秒生成令牌数（速率）
#define TBF_BURST       2000         // 令牌桶最大容量（突发流量上限）

/* 线程池参数 */
#define THREAD_POOL_SIZE 20          // 线程池中工作线程的数量

/* 进程池参数 */
#define PROCESS_POOL_SIZE 4          // 进程池中工作子进程的数量

/* 共享内存大小 */
#define SHM_SIZE        4096         // 共享内存区域大小（字节）

/* 消息队列键值 */
#define MSG_KEY         0x1234       // System V 消息队列的键值

/* 信号量键值 */
#define SEM_KEY         0x5678       // System V 信号量数组的键值

/* 全局标志（外部声明，实际定义在某个.c文件中） */
extern volatile sig_atomic_t g_shutdown;   // 原子退出标志，收到SIGTERM/SIGINT时置1
extern int g_listen_fd;                    // TCP监听套接字文件描述符
extern int g_udp_fd;                       // UDP套接字文件描述符
extern int g_signal_pipe[2];               // 信号管道（用于将信号转换为IO事件）

/* 函数声明（各模块对外接口） */

// daemon.c
void daemonize(void);                      // 将进程变为守护进程
void signal_init(void);                    // 注册信号处理函数（SIGTERM, SIGINT, SIGHUP, SIGPIPE）
void pidfile_create(void);                 // 创建PID文件并加锁，防止多实例
void pidfile_remove(void);                 // 删除PID文件（程序退出时调用）

// config.c
void config_load(const char *path);        // 加载配置文件（一次性读取所有配置项）

// log.c
void log_init(const char *logdir);         // 初始化日志系统（创建日志目录，打开日志文件）
void log_write(int level, const char *fmt, ...); // 写入日志（支持级别和格式化输出）

// network.c (或由其他模块实现)
void tcp_server_init(void);                // 初始化TCP服务器（创建socket，绑定，监听）
void udp_server_init(void);                // 初始化UDP服务器（创建socket，绑定）

// io_multiplexing.c
void poll_loop(void);                      // poll主事件循环（永不返回）

// signal_handler.c (或由main.c实现)
void graceful_shutdown(int signo);         // 优雅退出信号处理函数
void reload_config(int signo);             // 重新加载配置信号处理函数（SIGHUP）

/* 云平台上报模块接口（cloud_api.c） */
int cloud_api_init(void);                  // 初始化云API模块（如分配资源）
void cloud_api_upload(const char *sensor_id, const char *value); // 上传传感器数据到云平台
void cloud_api_cleanup(void);              // 清理云API模块资源

/* 云平台 HTTP 配置（外部全局变量，实际定义在config.c） */
/* extern char g_cloud_api_url[256]; */    // 云API URL（已注释，可能未使用）
extern char g_cloud_api_token[256];        // 云平台API Token（原始密钥）
extern int g_gateway_id;                   // 网关ID
extern int g_device_id;                    // 设备ID

/* 云平台传感器ID配置（外部全局变量，实际定义在config.c） */
extern char g_sht11_humi_id[32];   // SHT11湿度传感器ID（云平台标识，值如"16816"）
extern char g_sht11_temp_id[32];   // SHT11温度传感器ID（云平台标识，值如"16817"）
extern char g_adc_sensor_id[32];   // ADC光敏传感器ID（云平台标识，值如"16828"）
// 不再使用 DHT11、SHT30 相关 ID

#endif   // __IOT_GATEWAY_H 结束头文件保护
