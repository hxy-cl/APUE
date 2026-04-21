#include "iot_gateway.h"        // 项目公共头文件（包含日志、信号等）
#include "process_pool.h"       // 进程池模块接口声明
#include <sys/wait.h>           // 提供 waitpid 等进程等待函数

static int pool_size;           // 进程池大小（子进程数量）
static pid_t *pids;             // 存储每个子进程的 PID 的动态数组
static int *fifo_fds;           // 存储每个子进程对应命名管道写端文件描述符的数组
static process_job_t user_job;  // 用户提供的子进程工作函数（在子进程中调用）

// SIGCHLD 信号处理函数：自动收割僵尸子进程
// 参数: sig - 信号编号（未使用）
// 返回值: 无
static void sigchld_handler(int sig) {
    (void)sig;                  // 消除未使用参数警告
    // 循环调用 waitpid，WNOHANG 表示非阻塞，回收所有已终止的子进程
    while (waitpid(-1, NULL, WNOHANG) > 0);
}

// 初始化进程池
// 参数: num - 子进程数量
//       job - 子进程工作函数
// 返回值: 成功返回 0，失败返回 -1
int process_pool_init(int num, process_job_t job) {
    int i;                      // 循环变量
    pool_size = num;            // 保存进程池大小
    user_job = job;             // 保存用户工作函数

    // 动态分配存储子进程 PID 的数组
    pids = malloc(num * sizeof(pid_t));
    // 动态分配存储每个子进程对应管道写端 fd 的数组
    fifo_fds = malloc(num * sizeof(int));

    // 注册 SIGCHLD 信号处理函数，避免僵尸进程
    signal(SIGCHLD, sigchld_handler);

    // 循环创建 num 个子进程
    for (i = 0; i < num; i++) {
        char fifo_path[64];     // 命名管道路径缓冲区
        // 生成唯一的管道路径，例如 /tmp/iot_fifo_0
        snprintf(fifo_path, sizeof(fifo_path), "/tmp/iot_fifo_%d", i);
        unlink(fifo_path);      // 删除可能存在的旧管道文件

        // 创建命名管道（FIFO），权限 0666（读写）
        if (mkfifo(fifo_path, 0666) == -1) {
            perror("mkfifo");   // 创建失败，打印错误
            return -1;          // 初始化失败
        }

        pid_t pid = fork();     // 创建子进程
        if (pid == 0) {
            // ---------- 子进程代码 ----------
            // 以只读方式打开命名管道（等待父进程打开写端后才会返回）
            int fd = open(fifo_path, O_RDONLY);
            if (fd < 0) {
                perror("child open fifo");
                exit(1);        // 打开失败，子进程退出
            }
            // 调用用户提供的工作函数，传入管道读端 fd
            user_job(fd);
            close(fd);          // 关闭管道
            exit(0);            // 子进程正常退出
        } else {
            // ---------- 父进程代码 ----------
            pids[i] = pid;      // 保存子进程 PID
            // 父进程以只写方式打开命名管道（阻塞直到子进程打开读端）
            fifo_fds[i] = open(fifo_path, O_WRONLY);
            if (fifo_fds[i] < 0) {
                perror("open fifo");
                return -1;      // 打开失败，初始化失败
            }
        }
    }
    return 0;   // 初始化成功
}

// 向指定索引的子进程发送数据
// 参数: idx  - 子进程索引（0 ~ pool_size-1）
//       data - 数据指针
//       len  - 数据长度
// 返回值: 无
void process_pool_send(int idx, const void *data, size_t len) {
    // 索引越界检查
    if (idx < 0 || idx >= pool_size) return;
    // 通过命名管道向子进程写入数据
    if (write(fifo_fds[idx], data, len) < 0) {
        perror("write to fifo");   // 写入失败，打印错误
    }
}

// 销毁进程池：关闭管道、终止子进程、删除管道文件、释放内存
// 参数: 无
// 返回值: 无
void process_pool_destroy(void) {
    int i;                      // 循环变量
    for (i = 0; i < pool_size; i++) {
        if (fifo_fds[i] >= 0) close(fifo_fds[i]);   // 关闭管道写端
        if (pids[i] > 0) kill(pids[i], SIGTERM);    // 发送 SIGTERM 终止子进程
        char fifo_path[64];
        snprintf(fifo_path, sizeof(fifo_path), "/tmp/iot_fifo_%d", i);
        unlink(fifo_path);      // 删除命名管道文件
    }
    free(pids);                 // 释放 PID 数组内存
    free(fifo_fds);             // 释放管道 fd 数组内存
}
