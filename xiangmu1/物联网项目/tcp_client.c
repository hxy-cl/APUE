#include <stdio.h>          // 标准输入输出（perror, puts）
#include <sys/types.h>      // 基本系统数据类型（socket 相关）
#include <sys/socket.h>     // socket 编程接口
#include <netinet/in.h>     // 网络地址结构（sockaddr_in）
#include <arpa/inet.h>      // 地址转换函数（inet_aton）
#include <unistd.h>         // 系统调用（close, read）

#include "protocol.h"       // 项目协议头文件（定义 SERVER_IP, SERVER_PORT, MSGSIZE）

int main(void)
{
    int tcp_socket;                 // 存储成功创建的流式套接字描述符
    struct sockaddr_in raddr;       // 存储对端（服务器）的地址结构
    char msg[MSGSIZE] = {0};        // 接收缓冲区，存放从服务器读取的数据

    // 【1】创建 TCP 套接字（IPv4，流式，默认协议）
    tcp_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (tcp_socket == -1)           // 创建失败
    {
        perror("socket");           // 打印错误信息到标准错误
        return -1;                  // 程序退出，返回 -1
    }

    // 【2】设置服务器地址结构，准备 connect
    raddr.sin_family = AF_INET;                     // IPv4 协议族
    inet_aton(SERVER_IP, &raddr.sin_addr);          // 将点分十进制 IP 字符串转换为网络字节序的二进制地址
    raddr.sin_port = htons(SERVER_PORT);            // 将主机字节序的端口号转换为网络字节序

    // 请求连接到服务器
    if (connect(tcp_socket, (struct sockaddr *)&raddr, sizeof(raddr)) == -1)
    {
        perror("connect()");        // 连接失败，打印错误
        close(tcp_socket);          // 关闭套接字
        return -2;                  // 退出
    }

    // 【3】连接成功，进行 I/O 操作：从服务器读取数据
    read(tcp_socket, msg, MSGSIZE); // 读取最多 MSGSIZE 字节到 msg 缓冲区（假设服务器会发送数据）

    puts(msg);                      // 将读取到的数据打印到标准输出（自动换行）

    // 【4】关闭套接字
    close(tcp_socket);

    return 0;       // 正常退出
}
