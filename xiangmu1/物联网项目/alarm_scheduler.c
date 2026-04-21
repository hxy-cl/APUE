#include "iot_gateway.h"        // 包含物联网网关相关头文件（可能提供系统依赖）
#include "alarm_scheduler.h"    // 包含本模块的公开接口定义
#include <sys/time.h>           // 提供setitimer、itimerval等定时器操作

// 内部定时器节点结构体，用于链表管理每个周期任务
typedef struct timer_node {
    int interval;               // 任务周期（秒），固定不变
    int remaining;              // 剩余秒数，每触发一次SIGALRM减1
    timer_callback_t cb;        // 超时回调函数指针
    void *arg;                  // 回调函数的参数
    struct timer_node *next;    // 指向下一个节点，构成单链表
} timer_node_t;

static timer_node_t *timer_list = NULL;   // 定时器链表头指针，初始为空
static struct itimerval old_tv;           // 保存旧的定时器状态，用于销毁时恢复

// SIGALRM信号处理函数
// 参数: sig - 信号编号（此处为SIGALRM），函数内部未使用
// 返回值: 无
static void alarm_handler(int sig) {
    (void)sig;  // 消除未使用参数警告

    // 遍历整个定时器链表
    timer_node_t *p = timer_list;
    while (p) {
        p->remaining--;                     // 每个任务剩余时间减1秒
        if (p->remaining == 0) {            // 如果剩余时间为0，表示任务到期
            p->cb(p->arg);                  // 调用用户的回调函数，传递其参数
            p->remaining = p->interval;     // 重置剩余时间为周期值，实现周期性触发
        }
        p = p->next;                        // 继续处理下一个节点
    }
}

// 初始化闹钟调度器：注册信号处理函数，启动1秒间隔的硬件定时器
// 参数: 无
// 返回值: 成功返回0，失败返回-1（setitimer调用失败）
int alarm_scheduler_init(void) {
    // 注册SIGALRM信号的处理函数为alarm_handler
    signal(SIGALRM, alarm_handler);

    // 定义并配置itimerval结构体（注意：原代码拼写为itimervval，实际应为itimerval）
    struct itimerval tv;
    // 设置定时器间隔：每秒触发一次（即每隔1秒产生SIGALRM信号）
    tv.it_interval.tv_sec = 1;
    tv.it_interval.tv_usec = 0;
    // 设置首次触发时间：1秒后立即开始
    tv.it_value.tv_sec = 1;
    tv.it_value.tv_usec = 0;

    // 启动真实时间定时器（ITIMER_REAL），并保存旧的定时器状态到old_tv
    return setitimer(ITIMER_REAL, &tv, &old_tv);
}

// 向调度器中添加一个周期性任务
// 参数: interval_sec - 任务周期（秒）
//       cb - 到期时调用的回调函数
//       arg - 传给回调函数的参数
// 返回值: 成功返回0，失败返回-1（内存分配失败）
int alarm_add_task(int interval_sec, timer_callback_t cb, void *arg) {
    // 动态分配一个新的定时器节点
    timer_node_t *node = malloc(sizeof(timer_node_t));
    if (!node) return -1;               // 分配失败，返回-1

    // 初始化节点成员
    node->interval = interval_sec;      // 保存周期
    node->remaining = interval_sec;     // 剩余时间初始化为完整周期
    node->cb = cb;                      // 保存回调函数指针
    node->arg = arg;                    // 保存回调参数
    node->next = timer_list;            // 新节点指向原链表头（头插法）

    timer_list = node;                  // 更新链表头为新节点
    return 0;                           // 添加成功
}

// 销毁闹钟调度器：恢复旧定时器设置，释放所有动态分配的任务节点
// 参数: 无
// 返回值: 无
void alarm_scheduler_destroy(void) {
    // 恢复系统定时器到初始化之前的状态（通过old_tv）
    setitimer(ITIMER_REAL, &old_tv, NULL);

    // 遍历并释放链表中的所有节点
    while (timer_list) {
        timer_node_t *tmp = timer_list;   // 暂存当前头节点
        timer_list = timer_list->next;    // 头指针后移
        free(tmp);                        // 释放原头节点内存
    }
}
