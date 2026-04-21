#ifndef __TBF_H                // 头文件保护宏，防止重复包含
#define __TBF_H

#define PTHREAD                // 定义 PTHREAD 宏，表示使用多线程版本的令牌桶（使用互斥锁和条件变量）
                               // 如果注释掉此行，则使用信号版本（基于 SIGALRM 和 pause）

/*
 * 功能：初始化令牌桶
 * 参数: cps   - 每秒生成的令牌数（token per second），即速率
 *       burst - 令牌桶的最大容量（突发上限）
 * 返回值：成功返回令牌桶描述符（非负整数），失败返回 -1（参数错误）、-2（桶已满）、-3（内存分配失败）
 */
int tbf_init(int cps, int burst);

/*
 * 功能：从令牌桶中取出指定数量的令牌（非阻塞等待，若令牌不足则阻塞直到足够）
 * 参数: td - 令牌桶描述符（由 tbf_init 返回）
 *       n  - 需要取出的令牌数
 * 返回值：成功返回实际取出的令牌数（通常等于 n，除非桶中令牌不足时也会返回当前可用的全部令牌），
 *         失败返回 -1（td 无效）或 -2（桶已被销毁）
 */
int tbf_fetch_token(int td, int n);

/*
 * 功能：销毁指定的令牌桶，释放资源
 * 参数: td - 令牌桶描述符
 * 返回值：成功返回 0，失败返回 -1（td 无效）或 -2（桶已被销毁）
 */
int tbf_destroy(int td);

#if defined(PTHREAD)           // 以下函数仅在定义了 PTHREAD 宏时才存在（多线程版本）

/*
 * 功能：向令牌桶中返还令牌（用于某些场景下用不完的令牌可以归还）
 * 参数: td     - 令牌桶描述符
 *       ntoken - 要返还的令牌数
 * 返回值：成功返回 0，失败返回 -1（参数无效）或 -2（桶已被销毁）
 */
int tbf_return_token(int id, int ntoken);   // 注意参数名 id 实际应为 td，保持与头文件一致

/*
 * 功能：销毁所有已创建的令牌桶（通常用于程序退出时的清理）
 * 参数：无
 * 返回值：无
 */
void tbf_destroy_all(void);

#endif   // PTHREAD

#endif   // __TBF_H
