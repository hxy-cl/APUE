#ifndef __POOL_H                // 头文件保护宏，防止重复包含
#define __POOL_H

#include <pthread.h>            // 线程相关（pthread_t, pthread_mutex_t, pthread_cond_t）
#include "queue.h"              // 队列模块（提供队列结构及操作接口）

// 配置宏（可根据需要调整）
#define MAXJOB 1000     // 任务队列最大容量（测试版）
#define MIN_FREE_NR 5   // 线程池中最小空闲线程个数（保持活跃的线程数）
#define MAX_FREE_NR 20  // 线程池中最大空闲线程个数（超过此值会销毁多余线程）
#define STEP 2          // 每次增加或减少线程的数量（调整步长）

// 【1】定义线程池结构体
typedef struct
{
    pthread_t *workers;          // 工作线程 ID 数组的起始地址（动态开辟）
    pthread_t admin_tid;         // 管理者线程 ID
    queue_t *task_queue;         // 任务队列（存放待执行的任务）

    // 线程池控制参数
    int max_threads;             // 线程池允许的最大线程数量（容量上限）
    int min_free_threads;        // 最小空闲线程个数（线程池初始化时创建这么多）
    int max_free_threads;        // 最大空闲线程个数（超过此值则逐步销毁空闲线程）
    int busy_threads;            // 当前正在执行任务的线程数（忙线程数）
    int live_threads;            // 当前存活的线程总数（包括忙和空闲）
    int exit_threads;            // 需要终止的线程数量（管理者线程设置，工作线程自我终止）
    int shutdown;                // 线程池关闭标志（0: 正常运行, 1: 正在关闭）

    // 同步原语
    pthread_mutex_t mut_pool;    // 保护线程池整体数据（任务队列、控制变量）的互斥锁
    pthread_mutex_t mut_busy;    // 专门保护 busy_threads 的互斥锁（减少锁粒度）
    pthread_cond_t queue_not_empty; // 条件变量：任务队列非空时通知工作线程取任务
    pthread_cond_t queue_not_full;  // 条件变量：任务队列非满时通知主线程添加新任务
} pool_t;

// 任务结构体
typedef struct
{
    void *(*job)(void *arg);    // 任务函数指针（参数为 void*，返回 void*）
    void *arg;                  // 传递给任务函数的参数
} task_t;

// 【2】线程池接口函数声明

/*
功能：初始化线程池
参数：mypool   - 输出参数，用于返回创建的线程池指针
     capacity - 线程池最大线程容量（max_threads）
返回值：成功返回 0；失败返回负数（-1 内存分配失败，-2 工作线程数组分配失败，-err 线程创建失败）
说明：创建管理者线程和初始数量的工作线程，初始化任务队列和同步变量。
*/
extern int pool_init(pool_t **mypool, int capacity);

/*
功能：向线程池中添加一个任务
参数：mypool - 目标线程池指针
      t      - 指向任务结构体的指针（包含函数指针和参数）
返回值：成功返回 0；失败返回负数（当前实现总是返回 0）
说明：任务会被放入任务队列，由空闲工作线程执行。如果队列已满，调用者会阻塞等待。
*/
extern int pool_add_task(pool_t *mypool, const task_t *t);

/*
功能：销毁线程池，释放所有资源
参数：mypool - 要销毁的线程池指针
返回值：无
说明：设置 shutdown 标志，唤醒所有工作线程，等待它们退出，然后释放内存。
*/
extern void pool_destroy(pool_t *mypool);

#endif   // __POOL_H 结束头文件保护
