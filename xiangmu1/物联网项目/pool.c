#include <stdio.h>      // 标准输入输出（fprintf, stderr）
#include "pool.h"       // 线程池模块头文件
#include <stdlib.h>     // 标准库（malloc, free, calloc）
#include <string.h>     // 字符串操作（memset）
#include <unistd.h>     // POSIX 系统调用（sleep）
#include <signal.h>     // 信号相关（pthread_kill 使用的信号）
#include <errno.h>      // 错误码（ESRCH）

// 静态函数声明（仅本文件使用）
static void *admin_job(void *arg);   // 管理者线程入口函数
static void *worker_job(void *arg);  // 工作线程入口函数

// 初始化线程池
// 参数: mypool   - 输出参数，创建成功的线程池指针
//       capacity - 最大线程数量
// 返回值: 成功返回 0，失败返回负数
int pool_init(pool_t **mypool, int capacity)
{
    int err = 0;                // 存储线程创建返回值
    pool_t *me = NULL;          // 新创建的线程池指针
    int i = 0;                  // 循环变量

    // 为线程池结构体分配内存
    me = malloc(sizeof(pool_t));
    if (me == NULL)             // 内存分配失败
        return -1;

    // 动态分配工作线程 ID 数组（capacity 个 pthread_t 元素）
    me->workers = calloc(capacity, sizeof(pthread_t));
    if (me->workers == NULL)    // 分配失败
    {
        free(me);               // 释放已分配的线程池结构体
        return -2;              // 返回 -2 表示工作线程数组分配失败
    }

    // 初始化任务队列（队列元素类型为 task_t，最大容量 MAXJOB）
    queue_init(&me->task_queue, MAXJOB, sizeof(task_t));

    // 设置线程池参数
    me->max_threads = capacity;              // 最大线程数
    me->min_free_threads = MIN_FREE_NR;      // 最小空闲线程数（初始存活线程数）
    me->max_free_threads = MAX_FREE_NR;      // 最大空闲线程数（超过则销毁）
    me->busy_threads = 0;                    // 初始无忙线程
    me->live_threads = me->min_free_threads; // 初始存活线程数 = 最小空闲线程数
    me->exit_threads = 0;                    // 需要退出的线程数初始为0
    me->shutdown = 0;                        // 线程池运行标志（0:运行中）

    // 初始化互斥量和条件变量
    pthread_mutex_init(&me->mut_pool, NULL);       // 线程池主互斥锁
    pthread_mutex_init(&me->mut_busy, NULL);       // 保护 busy_threads 的互斥锁
    pthread_cond_init(&me->queue_not_empty, NULL); // 队列非空条件变量
    pthread_cond_init(&me->queue_not_full, NULL);  // 队列非满条件变量

    // 将工作线程数组所有元素初始化为 -1（表示该位置未使用或线程已退出）
    memset(me->workers, -1, capacity * sizeof(pthread_t));

    // 创建管理者线程
    err = pthread_create(&me->admin_tid, NULL, admin_job, me);
    if (err != 0)                               // 创建失败
    {
        free(me->workers);                      // 释放工作线程数组
        free(me);                               // 释放线程池结构体
        return -err;                            // 返回负的错误码
    }

    // 创建初始数量的工作线程（min_free_threads 个）
    for (i = 0; i < me->min_free_threads; i++)
    {
        err = pthread_create(me->workers + i, NULL, worker_job, me);
        if (err != 0)                           // 创建失败
        {
            free(me->workers);                  // 释放资源
            free(me);
            return -err;
        }
        // 将工作线程设置为分离状态（线程结束时自动回收资源，无需 join）
        pthread_detach(me->workers[i]);
    }

    *mypool = me;       // 将创建成功的线程池地址回填给调用者
    return 0;           // 初始化成功
}

// 在工作线程数组中查找一个空闲位置（可用来存放新线程ID）
// 参数: jobs - 工作线程ID数组
//       n    - 数组长度
// 返回值: 成功返回空闲位置索引（0 ~ n-1），失败返回 -1
static int __get_free_pos(pthread_t *jobs, int n)
{
    int i = 0;  // 循环变量

    // 第一轮：查找值为 (pthread_t)-1 的位置（表示从未使用或已标记空闲）
    for(i = 0; i < n; i++)
        if(jobs[i] == (pthread_t)-1)
            return i;

    // 第二轮：检查线程是否已经不存在（pthread_kill 返回 ESRCH 表示线程已退出）
    for(i = 0; i < n; i++)
    {
        // pthread_kill 发送信号 0 不会真正杀线程，只检查线程是否存在
        if(pthread_kill(jobs[i], 0) == ESRCH)
            return i;           // 线程已不存在，该位置可用
    }

    return -1;      // 没有空闲位置
}

// 管理者线程的主函数（负责动态调整线程数量）
// 参数: arg - 指向线程池结构体的指针
// 返回值: 无（线程退出时返回 NULL）
void *admin_job(void *arg)
{
    pool_t *mypool = (pool_t *)arg;     // 将参数转换为线程池指针
    int busy_cnt = 0;                   // 当前忙线程数量
    int free_cnt = 0;                   // 当前空闲线程数量
    int i = 0;                          // 循环变量
    int pos = 0;                        // 空闲位置索引

    // 管理者线程循环运行，定期检查线程池负载
    while(1)
    {
        // 加锁保护线程池整体数据
        pthread_mutex_lock(&mypool->mut_pool);
        if(mypool->shutdown)            // 如果线程池已标记关闭
        {
            pthread_mutex_unlock(&mypool->mut_pool); // 解锁
            break;                      // 退出循环，结束管理者线程
        }

        // 获取当前忙线程数量（加锁保护 busy_threads）
        pthread_mutex_lock(&mypool->mut_busy);
        busy_cnt = mypool->busy_threads;
        pthread_mutex_unlock(&mypool->mut_busy);

        // 计算空闲线程数 = 存活线程数 - 忙线程数
        free_cnt = mypool->live_threads - busy_cnt;

        // 如果空闲线程数 >= (最大空闲线程数 + 步长)，则需要销毁一些空闲线程
        if(free_cnt >= mypool->max_free_threads + STEP)
        {
            mypool->exit_threads = STEP;        // 设置需要退出的线程数为 STEP
            for(i = 0; i < STEP; i++)
            {
                // 向队列非空条件变量发送信号，唤醒工作线程
                // 工作线程被唤醒后会检查 exit_threads 并自我终止
                pthread_cond_signal(&mypool->queue_not_empty);
            }
        }

        // 如果忙线程数等于存活线程数（即没有空闲线程）且尚未达到最大线程数，则需要增加线程
        if(busy_cnt == mypool->live_threads && mypool->live_threads < mypool->max_threads)
        {
            for(i = 0; i < STEP; i++)
            {
                // 查找一个空闲位置来存放新线程的 ID
                pos = __get_free_pos(mypool->workers, mypool->max_threads);
                if(pos == -1)   // 没有空闲位置（理论上不会发生，因为 live_threads < max_threads）
                {
                    fprintf(stderr, "[%d]__get_free_pos is Failed!\n", __LINE__);
                    break;
                }
                // 创建新的工作线程
                pthread_create(mypool->workers + pos, NULL, worker_job, mypool);
                // 分离新线程
                pthread_detach(mypool->workers[pos]);
                // 存活线程数加1
                mypool->live_threads++;
            }
        }

        pthread_mutex_unlock(&mypool->mut_pool); // 解锁线程池
        sleep(1);   // 管理者线程每1秒检查一次
    }
    pthread_exit(0);    // 终止管理者线程
}

// 工作线程的主函数（从任务队列取任务并执行）
// 参数: arg - 指向线程池结构体的指针
// 返回值: 无（线程退出时返回 NULL）
static void *worker_job(void *arg)
{
    pool_t *mypool = (pool_t *)arg;     // 转换为线程池指针
    task_t mytask;                      // 存储从队列中取出的任务

    while(1)
    {
        // 加锁保护线程池数据
        pthread_mutex_lock(&mypool->mut_pool);

        // 如果任务队列为空，则等待队列非空条件变量
        if(queue_is_empty(mypool->task_queue))
        {
            pthread_cond_wait(&mypool->queue_not_empty, &mypool->mut_pool);
        }

        // 检查线程池是否正在关闭
        if(mypool->shutdown)
        {
            pthread_mutex_unlock(&mypool->mut_pool);
            break;      // 退出工作线程
        }

        // 检查是否需要退出（管理者线程要求销毁线程）
        if(mypool->exit_threads > 0)
        {
            mypool->exit_threads--;     // 减少待退出计数
            mypool->live_threads--;     // 存活线程数减1
            pthread_mutex_unlock(&mypool->mut_pool);
            break;      // 退出工作线程
        }

        // 从任务队列取出一个任务（出队）
        queue_de(mypool->task_queue, &mytask);
        // 发送队列非满信号（通知可能正在等待添加任务的线程）
        pthread_cond_signal(&mypool->queue_not_full);
        pthread_mutex_unlock(&mypool->mut_pool); // 解锁

        // 执行任务前，将自身标记为忙线程
        pthread_mutex_lock(&mypool->mut_busy);
        mypool->busy_threads++;
        pthread_mutex_unlock(&mypool->mut_busy);

        // 执行任务（调用任务函数）
        (mytask.job)(mytask.arg);

        // 任务执行完毕，将自身标记为空闲
        pthread_mutex_lock(&mypool->mut_busy);
        mypool->busy_threads--;
        pthread_mutex_unlock(&mypool->mut_busy);
    }

    pthread_exit(0);    // 终止工作线程
}

// 向线程池中添加一个任务
// 参数: mypool - 目标线程池
//       t      - 任务结构体指针
// 返回值: 成功返回 0
int pool_add_task(pool_t *mypool, const task_t *t)
{
    // 加锁保护线程池数据
    pthread_mutex_lock(&mypool->mut_pool);

    // 如果任务队列已满，则等待队列非满条件变量
    while(queue_is_full(mypool->task_queue))
    {
        pthread_cond_wait(&mypool->queue_not_full, &mypool->mut_pool);
    }

    // 将任务入队
    queue_en(mypool->task_queue, t);
    // 发送队列非空信号，唤醒一个等待取任务的工作线程
    pthread_cond_signal(&mypool->queue_not_empty);
    pthread_mutex_unlock(&mypool->mut_pool);

    return 0;
}

// 销毁线程池，释放所有资源
// 参数: mypool - 要销毁的线程池
// 返回值: 无
void pool_destroy(pool_t *mypool)
{
    // 加锁并设置关闭标志
    pthread_mutex_lock(&mypool->mut_pool);
    mypool->shutdown = 1;       // 标记线程池关闭
    // 广播队列非空条件变量，唤醒所有阻塞的工作线程，让它们检查关闭标志并退出
    pthread_cond_broadcast(&mypool->queue_not_empty);
    pthread_mutex_unlock(&mypool->mut_pool);

    sleep(1);   // 等待工作线程退出（简单等待，实际可考虑 join 或更优雅的方式）

    // 释放动态分配的资源
    free(mypool->workers);               // 释放工作线程 ID 数组
    queue_destroy(mypool->task_queue);   // 销毁任务队列
    pthread_mutex_destroy(&mypool->mut_pool);       // 销毁互斥锁
    pthread_mutex_destroy(&mypool->mut_busy);       // 销毁 busy 互斥锁
    pthread_cond_destroy(&mypool->queue_not_empty); // 销毁条件变量
    pthread_cond_destroy(&mypool->queue_not_full);  // 销毁条件变量

    free(mypool);        // 释放线程池结构体本身
}
