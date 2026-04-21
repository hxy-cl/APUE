#ifndef __PROCESS_POOL_H        // 头文件保护宏，防止重复包含
#define __PROCESS_POOL_H

#include <sys/types.h>          // 提供 pid_t 等类型定义

// 子进程工作函数类型定义
// 参数: fd - 命名管道的文件描述符（子进程用于读取父进程命令）
// 返回值: 无
typedef void (*process_job_t)(int fd);

// 初始化进程池
// 参数: num - 子进程数量
//       job - 子进程执行的用户函数（每个子进程会调用此函数，传入 fifo 读端 fd）
// 返回值: 成功返回 0，失败返回 -1
int process_pool_init(int num, process_job_t job);

// 销毁进程池（关闭管道、终止子进程、清理资源）
// 参数: 无
// 返回值: 无
void process_pool_destroy(void);

// 向指定索引的子进程发送数据
// 参数: idx  - 子进程索引（0 ~ num-1）
//       data - 指向要发送的数据的指针
//       len  - 数据长度（字节）
// 返回值: 无
void process_pool_send(int idx, const void *data, size_t len);

#endif   // __PROCESS_POOL_H 结束头文件保护
