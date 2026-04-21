#include <stdio.h>          // 标准输入输出（printf, perror）
#include <sys/types.h>      // 基本系统数据类型
#include <sys/socket.h>     // socket 编程接口
#include <stdlib.h>         // 标准库（exit）
#include <string.h>         // 字符串操作（memset）
#include <netinet/in.h>     // 网络地址结构（sockaddr_in）
#include <netinet/ip.h>     // IP 协议相关（本例未直接使用）
#include <arpa/inet.h>      // 地址转换函数（inet_ntoa）
#include <unistd.h>         // 系统调用（close）

#define PORT 8889           // UDP 服务器监听的端口号
#define BUFFER_SIZE 512     // 接收缓冲区大小

int main(void)
{
    int sd = 0;                         // 套接字文件描述符
    struct sockaddr_in server_addr;     // 服务器本地地址结构
    struct sockaddr_in client_addr;     // 客户端地址结构（用于 recvfrom 获取发送端信息）
    socklen_t client_len = sizeof(client_addr); // 客户端地址长度
    char buffer[BUFFER_SIZE];           // 接收数据缓冲区
    int recv_len;                       // 实际接收到的字节数

    // 1. 创建 UDP 套接字（IPv4，数据报）
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd == -1)                       // 创建失败
    {
        perror("socket()");
        return -1;                      // 退出，返回 -1
    }

    // 2. 初始化服务器地址结构
    memset(&server_addr, 0, sizeof(server_addr));   // 清空结构体
    server_addr.sin_family = AF_INET;               // IPv4
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 监听所有本地网络接口（0.0.0.0）
    server_addr.sin_port = htons(PORT);             // 绑定端口号（转换为网络字节序）

    // 3. 绑定地址到套接字
    if (bind(sd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("bind()");
        close(sd);          // 关闭套接字
        return -2;          // 退出，返回 -2
    }

    // 4. 打印启动信息
    printf("UDP Server listening on port %d...\n", PORT);

    // 5. 循环接收客户端数据（永不退出）
    while (1)
    {
        // 接收数据报（阻塞）
        recv_len = recvfrom(sd, buffer, BUFFER_SIZE - 1, 0,
                           (struct sockaddr *)&client_addr, &client_len);
        if (recv_len == -1)             // 接收出错
        {
            perror("recvfrom()");
            continue;                   // 跳过本次循环，继续等待下一个数据报
        }

        buffer[recv_len] = '\0';        // 添加字符串结束符，以便打印
        // 打印收到的数据信息：客户端 IP、端口、字节数、内容
        printf("Received from %s:%d [%d bytes]: %s\n",
               inet_ntoa(client_addr.sin_addr),   // 将二进制 IP 转换为点分十进制字符串
               ntohs(client_addr.sin_port),       // 端口号转为主机字节序
               recv_len, buffer);
    }

    // 6. 关闭套接字（实际永远不会执行到，因为上面是无限循环）
    close(sd);
    return 0;
}
