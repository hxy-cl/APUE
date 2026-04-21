#include <stdio.h>          // 标准输入输出（perror）
#include <sys/socket.h>     // socket 编程接口
#include <netinet/in.h>     // 网络地址结构，TCP 头文件（包含 TCP_NODELAY 等）
#include <netinet/tcp.h>    // TCP 选项（本例未直接使用，但包含）
#include <sys/types.h>      // 基本系统数据类型
#include <arpa/inet.h>      // 地址转换函数
#include <unistd.h>         // 系统调用（close, write）
#include <stdlib.h>         // 标准库（exit）
#include <signal.h>         // 信号处理（sigaction）

#include "protocol.h"       // 项目协议头文件（定义 SERVER_PORT）

int main(void)
{
    int tcp_socket;                 // 监听套接字
    int new_socket;                 // 已连接套接字（accept 返回）
    struct sockaddr_in laddr;       // 本地地址结构
    pid_t pid;                      // 子进程 PID
    struct sigaction act;           // 信号行为结构体

    // 配置 SIGCHLD 信号的处理行为，避免产生僵尸进程
    act.sa_handler = SIG_DFL;       // 信号处理函数设为默认（实际上 SA_NOCLDWAIT 标志下默认行为是忽略）
    act.sa_flags = SA_NOCLDWAIT;    // 关键标志：子进程终止时不会变为僵尸进程，且不等待子进程
    sigaction(SIGCHLD, &act, NULL); // 注册 SIGCHLD 信号的处理

    // 【1】创建 TCP 套接字（IPv4，流式）
    tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_socket == -1)
    {
        perror("socket");
        return -1;
    }

    // 【2】绑定本地地址
    laddr.sin_family = AF_INET;                 // IPv4
    laddr.sin_addr.s_addr = INADDR_ANY;         // 监听所有本地网络接口（相当于 "0.0.0.0"）
    laddr.sin_port = htons(SERVER_PORT);        // 端口号（来自 protocol.h）
    if (bind(tcp_socket, (struct sockaddr *)&laddr, sizeof(laddr)) == -1)
    {
        perror("bind()");
        close(tcp_socket);
        return -2;
    }

    // 【3】监听连接请求（最大等待队列长度 20）
    if (listen(tcp_socket, 20) == -1)
    {
        perror("listen()");
        close(tcp_socket);
        return -3;
    }

    // 【4】循环接受客户端连接
    while (1)
    {
        new_socket = accept(tcp_socket, NULL, NULL);   // 接受连接，不关心客户端地址
        if (new_socket == -1)
        {
            perror("accept()");
            close(tcp_socket);
            return -4;
        }
        pid = fork();                                   // 创建子进程处理该连接
        if (pid == -1)
        {
            perror("fork()");
            close(tcp_socket);
            return -5;
        }
        // 【5】子进程处理具体 I/O
        if (pid == 0)   // 子进程
        {
            write(new_socket, "Hello HXY", 10);         // 向客户端发送固定字符串
            close(new_socket);                          // 关闭已连接套接字
            close(tcp_socket);                          // 子进程不需要监听套接字，关闭它
            exit(0);                                    // 子进程退出
        }
        // 父进程：关闭已连接套接字（因为子进程已经接手），继续循环 accept
        close(new_socket);
    }

    // 【6】关闭监听套接字（实际上由于无限循环，永远不会执行到这里）
    close(tcp_socket);
    return 0;
}
