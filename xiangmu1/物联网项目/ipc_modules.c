/* 消息队列、共享内存、信号量模块实现 */
#include "iot_gateway.h"          // 包含项目公共头文件（错误码、日志等）
#include "ipc_modules.h"          // 包含本模块的接口声明
#include <sys/msg.h>              // 消息队列系统调用
#include <sys/shm.h>              // 共享内存系统调用
#include <sys/sem.h>              // 信号量系统调用

// 联合体，用于 semctl 的第四个参数（System V 信号量初始化所需）
union semun {
    int val;                      // 用于 SETVAL 命令，设置信号量的值
    struct semid_ds *buf;         // 用于 IPC_STAT/IPC_SET，指向 semid_ds 结构
    unsigned short *array;        // 用于 GETALL/SETALL，指向无符号短整型数组
};

// 创建或打开一个 System V 消息队列
// 参数: key - IPC键值
// 返回值: 成功返回消息队列ID，失败返回-1
int ipc_msg_queue_init(key_t key) {
    // msgget: 创建新队列（IPC_CREAT）或获取已有队列，权限0666（读写）
    int msqid = msgget(key, IPC_CREAT | 0666);
    if (msqid < 0) perror("msgget");   // 出错时打印错误信息到stderr
    return msqid;                       // 返回ID（可能为-1）
}

// 发送消息到消息队列
// 参数: msqid - 消息队列ID
//       type  - 消息类型（长整型，必须>0）
//       data  - 要发送的数据指针
//       len   - 数据长度（字节，不能超过系统限制如1024）
// 返回值: 成功返回0，失败返回-1
int ipc_msg_send(int msqid, long type, const void *data, size_t len) {
    // 定义消息缓冲区结构（必须包含 mtype 和 mtext）
    struct msgbuf {
        long mtype;                  // 消息类型（用户指定）
        char mtext[1024];            // 消息数据（最大1024字节，可根据需要调整）
    } buf;
    buf.mtype = type;                // 设置消息类型
    // 将用户数据复制到 mtext 中（注意：len 不能超过 sizeof(buf.mtext)）
    memcpy(buf.mtext, data, len);
    // msgsnd: 发送消息，flags=0 表示阻塞直到空间可用
    return msgsnd(msqid, &buf, len, 0);
}

// 从消息队列接收消息（阻塞）
// 参数: msqid - 消息队列ID
//       type  - 要接收的消息类型（0：任意类型；>0：指定类型；<0：取类型绝对值的最小值）
//       buf   - 接收缓冲区（用户提供）
//       len   - 缓冲区大小（字节）
// 返回值: 成功返回实际接收的字节数，失败返回-1
int ipc_msg_recv(int msqid, long type, void *buf, size_t len) {
    // 定义接收缓冲区结构（与发送端一致）
    struct msgbuf {
        long mtype;
        char mtext[1024];
    } rbuf;
    // msgrcv: 接收消息，flags=0 表示阻塞，不指定MSG_NOERROR（数据过长时截断）
    int ret = msgrcv(msqid, &rbuf, len, type, 0);
    if (ret > 0) {                   // 成功接收到数据
        memcpy(buf, rbuf.mtext, ret); // 将消息数据复制到用户缓冲区
    }
    return ret;                      // 返回字节数或-1
}

// 创建或打开共享内存段
// 参数: key  - IPC键值
//       size - 共享内存大小（字节）
// 返回值: 成功返回共享内存ID，失败返回-1
int ipc_shm_init(key_t key, size_t size) {
    // shmget: 创建新段（IPC_CREAT）或获取已有段，权限0666
    return shmget(key, size, IPC_CREAT | 0666);
}

// 将共享内存附加到当前进程地址空间
// 参数: shmid - 共享内存ID
// 返回值: 成功返回映射地址，失败返回 (void*)-1
void *ipc_shm_attach(int shmid) {
    // shmat: 附加共享内存，shmaddr=NULL由系统选择地址，flags=0表示可读写
    return shmat(shmid, NULL, 0);
}

// 从当前进程分离共享内存（不删除共享内存段）
// 参数: addr - 共享内存附加地址（由 ipc_shm_attach 返回）
// 返回值: 无
void ipc_shm_detach(void *addr) {
    shmdt(addr);   // shmdt: 分离共享内存
}

// 创建或打开一组信号量，并初始化为1（互斥锁）
// 参数: key   - IPC键值
//       nsems - 信号量个数
// 返回值: 成功返回信号量ID，失败返回-1
int ipc_sem_init(key_t key, int nsems) {
    // semget: 创建新集合（IPC_CREAT）或获取已有集合，权限0666
    int semid = semget(key, nsems, IPC_CREAT | 0666);
    if (semid < 0) return -1;        // 创建失败
    union semun arg;                 // semctl 的参数联合体
    // 遍历每个信号量，将其初始值设为1（互斥锁）
    for (int i = 0; i < nsems; i++) {
        arg.val = 1;                 // 设置值为1（未锁定状态）
        semctl(semid, i, SETVAL, arg); // 调用 semctl 设置信号量的值
    }
    return semid;                    // 返回信号量ID
}

// P操作（等待/减1），用于加锁
// 参数: semid   - 信号量ID
//       semnum  - 信号量在数组中的索引（从0开始）
// 返回值: 成功返回0，失败返回-1
int ipc_sem_p(int semid, int semnum) {
    struct sembuf op = {semnum, -1, 0}; // 操作：semnum号信号量减1，flags=0（阻塞）
    return semop(semid, &op, 1);        // 执行一个操作
}

// V操作（释放/加1），用于解锁
// 参数: semid   - 信号量ID
//       semnum  - 信号量在数组中的索引
// 返回值: 成功返回0，失败返回-1
int ipc_sem_v(int semid, int semnum) {
    struct sembuf op = {semnum, 1, 0};  // 操作：semnum号信号量加1
    return semop(semid, &op, 1);        // 执行一个操作
}
