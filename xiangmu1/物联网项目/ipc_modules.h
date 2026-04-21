#ifndef __IPC_MODULES_H            // 头文件保护宏：防止重复包含
#define __IPC_MODULES_H

#include <sys/types.h>             // 提供 size_t, key_t 等类型定义

// 消息队列接口
int ipc_msg_queue_init(key_t key);   // 创建或打开一个消息队列
// 参数: key - IPC键值（通常由ftok生成或使用固定值）
// 返回值: 成功返回消息队列ID，失败返回-1

int ipc_msg_send(int msqid, long type, const void *data, size_t len);
// 功能: 发送消息到消息队列
// 参数: msqid - 消息队列ID
//       type  - 消息类型（正数，用于接收时筛选）
//       data  - 消息数据指针
//       len   - 数据长度（字节）
// 返回值: 成功返回0，失败返回-1

int ipc_msg_recv(int msqid, long type, void *buf, size_t len);
// 功能: 从消息队列接收消息（阻塞）
// 参数: msqid - 消息队列ID
//       type  - 要接收的消息类型（0表示任意类型，>0表示指定类型）
//       buf   - 接收缓冲区
//       len   - 缓冲区大小
// 返回值: 成功返回实际接收的字节数，失败返回-1

// 共享内存接口
int ipc_shm_init(key_t key, size_t size);
// 功能: 创建或打开共享内存段
// 参数: key  - IPC键值
//       size - 共享内存大小（字节）
// 返回值: 成功返回共享内存ID，失败返回-1

void *ipc_shm_attach(int shmid);
// 功能: 将共享内存附加到当前进程地址空间
// 参数: shmid - 共享内存ID
// 返回值: 成功返回映射地址，失败返回(void*)-1

void ipc_shm_detach(void *addr);
// 功能: 从当前进程分离共享内存（不删除）
// 参数: addr - 共享内存附加地址（由ipc_shm_attach返回）
// 返回值: 无

// 信号量数组接口
int ipc_sem_init(key_t key, int nsems);
// 功能: 创建或打开一组信号量，并将所有信号量初始化为1（互斥锁）
// 参数: key   - IPC键值
//       nsems - 信号量个数
// 返回值: 成功返回信号量ID，失败返回-1

int ipc_sem_p(int semid, int semnum);
// 功能: P操作（等待/减1），用于加锁
// 参数: semid   - 信号量ID
//       semnum  - 信号量在数组中的索引（从0开始）
// 返回值: 成功返回0，失败返回-1

int ipc_sem_v(int semid, int semnum);
// 功能: V操作（释放/加1），用于解锁
// 参数: semid   - 信号量ID
//       semnum  - 信号量在数组中的索引
// 返回值: 成功返回0，失败返回-1

#endif   // __IPC_MODULES_H 结束头文件保护
