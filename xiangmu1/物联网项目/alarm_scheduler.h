#ifndef __ALARM_SCHEDULER_H   // 头文件保护宏：防止重复包含
#define __ALARM_SCHEDULER_H   // 定义宏，表示头文件已被包含

// 定时器回调函数类型定义
// 参数: arg - 用户自定义数据指针
// 返回值: 无
typedef void (*timer_callback_t)(void *arg);

// 初始化闹钟调度器模块
// 参数: 无
// 返回值: 成功返回0，失败返回-1（由setitimer决定）
int alarm_scheduler_init(void);

// 添加一个周期性的定时任务
// 参数: interval_sec - 定时周期（秒）
//       cb - 超时后调用的回调函数指针
//       arg - 传递给回调函数的参数
// 返回值: 成功返回0，失败返回-1（内存分配失败）
int alarm_add_task(int interval_sec, timer_callback_t cb, void *arg);

// 运行闹钟调度器主循环（注意：本声明在.c中未实现，预留接口）
// 参数: 无
// 返回值: 无
void alarm_scheduler_run(void);

// 销毁闹钟调度器，释放所有资源并恢复系统定时器状态
// 参数: 无
// 返回值: 无
void alarm_scheduler_destroy(void);

#endif   // __ALARM_SCHEDULER_H 结束头文件保护
