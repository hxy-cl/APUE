/* 守护进程化模块 */
#include "iot_gateway.h"   // 包含项目公共头文件（定义PID_FILE等）
#include <sys/file.h>      // 提供flock文件锁功能

/* 将当前进程转变为守护进程（Daemon） */
void daemonize(void) {
    pid_t pid;              // 进程ID

    // 第一次fork：脱离终端
    pid = fork();
    if (pid < 0) {
        perror("fork");     // 错误输出到标准错误（此时可能还未完全脱离）
        exit(1);            // 退出
    }
    if (pid > 0) exit(0);   // 父进程退出，子进程继续

    // 创建新会话，成为会话首进程（脱离控制终端）
    setsid();

    // 第二次fork：确保进程不是会话首进程，防止意外获取控制终端
    pid = fork();
    if (pid < 0) {
        perror("fork2");
        exit(1);
    }
    if (pid > 0) exit(0);   // 父进程退出，子进程（孙子进程）成为守护进程

    // 切换工作目录到根目录（避免占用可卸载文件系统）
    if (chdir("/") < 0) {}; // 失败时忽略（因为通常不会失败）

    // 重置文件创建掩码（确保守护进程创建的文件有明确权限）
    umask(0);

    // 关闭标准输入、标准输出、标准错误（继承自父进程的文件描述符）
    close(0); close(1); close(2);

    // 打开/dev/null作为标准输入（文件描述符0）
    open("/dev/null", O_RDWR);
    // 将标准输出（1）和标准错误（2）重定向到同一个/dev/null（即丢弃所有输出）
    dup2(0, 1);
    dup2(0, 2);
}

/* 创建PID文件并加锁，防止多个实例同时运行 */
void pidfile_create(void) {
    // 打开PID文件（读写方式，若不存在则创建，权限0644）
    int fd = open(PID_FILE, O_RDWR | O_CREAT, 0644);
    if (fd < 0) {
        perror("open pidfile");   // 打开失败，打印错误
        exit(1);                  // 退出程序
    }
    // 尝试对文件加排他锁（非阻塞模式），如果已有其他进程持有锁则立即失败
    if (flock(fd, LOCK_EX | LOCK_NB) < 0) {
        fprintf(stderr, "Already running\n"); // 已有实例运行
        exit(1);                              // 退出
    }
    // 将当前进程ID写入文件
    char buf[16];
    snprintf(buf, sizeof(buf), "%d\n", getpid()); // 格式化为字符串加换行
    if (write(fd, buf, strlen(buf)) < 0) {};      // 写入失败时忽略（不影响主逻辑）
    // 注意：文件描述符fd故意不关闭，以保持锁；进程退出时系统自动关闭
}

/* 删除PID文件（通常在程序正常退出时调用） */
void pidfile_remove(void) {
    unlink(PID_FILE);   // 删除PID文件（锁会自动释放）
}
