#include <stdio.h>      // 标准输入输出（fprintf, perror）
#include <signal.h>     // 信号处理（signal, alarm）
#include <unistd.h>     // POSIX 系统调用（sleep, pause）
#include <stdlib.h>     // 标准库（malloc, free, atexit, exit）
#include <pthread.h>    // 线程库（pthread_mutex_*，pthread_cond_*）
#include <string.h>     // 字符串操作（strerror）
#include "tbf.h"        // 令牌桶模块头文件

#define TBFMAX 1024     // 系统最多同时支持的令牌桶数量

// 令牌桶内部结构体（不对外暴露）
typedef struct tbf_st {
    int token;          // 当前桶中的令牌数
    int cps;            // 每秒生成令牌数（速率）
    int burst;          // 桶的最大容量（突发上限）
#ifdef PTHREAD          // 多线程版本需要互斥锁和条件变量
    pthread_mutex_t mutex;   // 保护本桶数据的互斥锁
    pthread_cond_t cond;     // 条件变量，用于等待令牌可用
#endif
} tbf_t;

static tbf_t *tbf_libs[TBFMAX];   // 全局指针数组，存储所有令牌桶对象，空闲位置为 NULL
static int initd = 0;             // 模块是否已初始化（0=未初始化，1=已初始化）

#ifdef PTHREAD    // ========== 多线程版本（使用单独的后台线程周期性增加令牌）==========
static pthread_mutex_t mut_job = PTHREAD_MUTEX_INITIALIZER; // 保护 tbf_libs 数组的互斥锁
static pthread_t jobtid;          // 后台工作线程 ID

// 后台线程函数：每隔 1 秒为所有存在的令牌桶增加令牌（cps 个），不超过 burst
// 参数: arg - 未使用
// 返回值: 无（线程退出返回 NULL）
static void *thr_fun(void *arg) {
    int i;
    (void)arg;          // 消除未使用参数警告
    while (1) {
        pthread_mutex_lock(&mut_job);                     // 加锁遍历数组
        for (i = 0; i < TBFMAX; i++) {
            if (tbf_libs[i] != NULL) {                    // 桶存在
                pthread_mutex_lock(&tbf_libs[i]->mutex);  // 锁住该桶
                tbf_libs[i]->token += tbf_libs[i]->cps;   // 增加 cps 个令牌
                if (tbf_libs[i]->token > tbf_libs[i]->burst) // 不超过容量
                    tbf_libs[i]->token = tbf_libs[i]->burst;
                pthread_mutex_unlock(&tbf_libs[i]->mutex); // 解锁桶
                pthread_cond_broadcast(&tbf_libs[i]->cond); // 唤醒所有等待该桶条件变量的线程（可能有多个取令牌的线程在等待）
            }
        }
        pthread_mutex_unlock(&mut_job);   // 解锁数组
        sleep(1);                         // 休眠 1 秒，周期性执行
    }
    return NULL;
}

// 模块卸载函数（通过 atexit 注册，在程序退出时自动调用）
// 功能：停止后台线程，销毁所有令牌桶
// 参数: 无
// 返回值: 无
static void module_unload(void) {
    int i;
    pthread_cancel(jobtid);      // 取消后台线程
    pthread_join(jobtid, NULL);  // 等待线程结束
    for (i = 0; i < TBFMAX; i++) {
        if (tbf_libs[i] != NULL)
            tbf_destroy(i);      // 销毁每个桶
    }
}

// 模块加载函数（在第一次调用 tbf_init 时执行）
// 功能：创建后台线程，注册退出清理函数
// 参数: 无
// 返回值: 无
static void module_load(void) {
    int ret = pthread_create(&jobtid, NULL, thr_fun, NULL); // 创建后台线程
    if (ret != 0) {
        fprintf(stderr, "pthread_create(): %s\n", strerror(ret));
        exit(1);                 // 创建失败，程序退出
    }
    atexit(module_unload);       // 注册退出时自动清理
}

#else   // ========== 非多线程版本（使用 SIGALRM 信号周期性增加令牌）==========
// SIGALRM 信号处理函数
// 参数: none - 信号编号（未使用）
// 返回值: 无
static void alarm_handler(int none) {
    int i;
    alarm(1);                     // 重新设置 1 秒后的闹钟，实现周期性
    for (i = 0; i < TBFMAX; i++) {
        if (tbf_libs[i] != NULL) {
            tbf_libs[i]->token += tbf_libs[i]->cps;   // 增加令牌
            if (tbf_libs[i]->token > tbf_libs[i]->burst)
                tbf_libs[i]->token = tbf_libs[i]->burst;
        }
    }
}

// 模块加载函数（信号版本）
// 功能：注册 SIGALRM 信号处理函数，启动闹钟
// 参数: 无
// 返回值: 无
static void module_load(void) {
    signal(SIGALRM, alarm_handler);   // 注册信号处理函数
    alarm(1);                         // 1 秒后触发第一次闹钟
}
#endif   // PTHREAD

// 查找一个空闲的令牌桶槽位（位置索引）
// 参数: 无
// 返回值: 成功返回空闲索引（0 ~ TBFMAX-1），失败（无空闲）返回 -1
static int get_tbf_pos(void) {
    int i;
    for (i = 0; i < TBFMAX; i++)
        if (tbf_libs[i] == NULL) return i;
    return -1;
}

// 初始化令牌桶（创建新桶）
// 参数: cps   - 每秒生成令牌数（必须 >0）
//       burst - 桶容量（必须 >0）
// 返回值: 成功返回桶描述符（非负整数索引），失败返回 -1（参数错误）、-2（桶满）、-3（内存分配失败）
int tbf_init(int cps, int burst) {
    int pos;
    if (cps <= 0 || burst <= 0) return -1;          // 参数无效
    if (!initd) {                                   // 首次调用，加载模块
        module_load();
        initd = 1;
    }
    pos = get_tbf_pos();                            // 获取空闲位置
    if (pos < 0) return -2;                         // 无空闲位置（桶满）
    tbf_libs[pos] = malloc(sizeof(tbf_t));          // 分配桶结构体
    if (tbf_libs[pos] == NULL) return -3;           // 内存分配失败
    // 初始化桶成员
    tbf_libs[pos]->cps = cps;
    tbf_libs[pos]->burst = burst;
    tbf_libs[pos]->token = 0;                       // 初始令牌数为 0（或可设为 burst？此处为 0）
#ifdef PTHREAD
    pthread_mutex_init(&tbf_libs[pos]->mutex, NULL); // 初始化互斥锁
    pthread_cond_init(&tbf_libs[pos]->cond, NULL);  // 初始化条件变量
#endif
    return pos;     // 返回桶描述符（即数组下标）
}

// 内联函数：取两个整数的最小值
static inline int min(int a, int b) { return a < b ? a : b; }

// 从令牌桶中取令牌（阻塞直到至少有一个令牌可用）
// 参数: td - 桶描述符
//       n  - 期望取出的令牌数（>0）
// 返回值: 成功返回实际取出的令牌数（通常等于 n，除非桶中令牌不足时会取出全部现有令牌），失败返回 -1 或 -2
int tbf_fetch_token(int td, int n) {
    int fetch = 0;
    if (td < 0 || td >= TBFMAX || n <= 0) return -1;        // 参数无效
    if (tbf_libs[td] == NULL) return -2;                    // 桶已被销毁
#ifdef PTHREAD
    // 多线程版本：使用互斥锁和条件变量
    pthread_mutex_lock(&tbf_libs[td]->mutex);
    while (tbf_libs[td]->token <= 0)                         // 没有令牌时等待
        pthread_cond_wait(&tbf_libs[td]->cond, &tbf_libs[td]->mutex);
    fetch = min(tbf_libs[td]->token, n);                     // 取 min(当前令牌数, 请求数)
    tbf_libs[td]->token -= fetch;                            // 减少令牌
    pthread_mutex_unlock(&tbf_libs[td]->mutex);
#else
    // 非多线程版本：使用 pause() 忙等（低效）
    while (tbf_libs[td]->token <= 0) pause();                // 等待信号（每次令牌增加会触发 SIGALRM，但 pause 可能被其他信号打断）
    fetch = min(tbf_libs[td]->token, n);
    tbf_libs[td]->token -= fetch;
#endif
    return fetch;
}

// 销毁指定的令牌桶
// 参数: td - 桶描述符
// 返回值: 成功返回 0，失败返回 -1 或 -2
int tbf_destroy(int td) {
    if (td < 0 || td >= TBFMAX) return -1;
    if (tbf_libs[td] == NULL) return -2;
#ifdef PTHREAD
    pthread_mutex_destroy(&tbf_libs[td]->mutex);   // 销毁互斥锁
    pthread_cond_destroy(&tbf_libs[td]->cond);     // 销毁条件变量
#endif
    free(tbf_libs[td]);            // 释放内存
    tbf_libs[td] = NULL;           // 指针置空
    return 0;
}

#ifdef PTHREAD   // 以下函数仅在多线程版本中提供
// 向令牌桶中返还令牌（例如用不完的令牌可以归还）
// 参数: td     - 桶描述符
//       ntoken - 要返还的令牌数（>0）
// 返回值: 成功返回 0，失败返回 -1 或 -2
int tbf_return_token(int td, int ntoken) {
    if (td < 0 || td >= TBFMAX || ntoken <= 0) return -1;
    if (tbf_libs[td] == NULL) return -2;
    pthread_mutex_lock(&tbf_libs[td]->mutex);
    tbf_libs[td]->token += ntoken;                       // 增加令牌
    if (tbf_libs[td]->token > tbf_libs[td]->burst)       // 不超过容量
        tbf_libs[td]->token = tbf_libs[td]->burst;
    pthread_mutex_unlock(&tbf_libs[td]->mutex);
    pthread_cond_broadcast(&tbf_libs[td]->cond);         // 唤醒所有等待令牌的线程
    return 0;
}

// 销毁所有令牌桶（释放全部资源）
// 参数: 无
// 返回值: 无
void tbf_destroy_all(void) {
    int i;
    pthread_mutex_lock(&mut_job);         // 锁住全局数组
    for (i = 0; i < TBFMAX; i++)
        if (tbf_libs[i] != NULL) tbf_destroy(i);   // 逐个销毁
    pthread_mutex_unlock(&mut_job);
}
#endif   // PTHREAD
