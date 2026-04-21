#include <stdio.h>          // 标准输入输出（perror, puts）
#include <sys/types.h>      // 基本系统数据类型
#include <sys/socket.h>     // socket 编程接口
#include <stdlib.h>         // 标准库（exit）
#include <string.h>         // 字符串操作（memset）
#include <netinet/in.h>     // 网络地址结构（sockaddr_in, IPPROTO_IP）
#include <netinet/ip.h>     // IP 协议相关（本例未直接使用）
#include <arpa/inet.h>      // 地址转换函数（inet_aton）
#include <unistd.h>         // 系统调用（close, sleep）
#include <net/if.h>         // 网络接口操作（if_nametoindex）

#include "protocol.h"       // 项目协议头文件（定义 MULTICAST_IP, LOCAL_IP, RECV_PORT, MSGSIZE, data_st）

int main(int argc, char *argv[])   // 程序入口（argc/argv 未使用）
{
    int sd = 0;                 // 套接字文件描述符
    struct data_st buf;         // 存储接收到的数据（使用协议定义的结构体）
    struct sockaddr_in my_addr; // 本地地址结构（用于 bind）
    struct ip_mreqn imr;        // 多播组选项结构（用于加入多播组）

    // [1] 创建 UDP 套接字（IPv4，数据报）
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd == -1)               // 创建失败
    {
        perror("socket()");     // 打印错误信息
        return -1;              // 退出，返回 -1
    }

    // [2] 将本地地址与套接字绑定（绑定到特定端口才能接收多播数据）
    my_addr.sin_family = AF_INET;                   // IPv4
    inet_aton(LOCAL_IP, &my_addr.sin_addr);         // 本地 IP（"0.0.0.0" 表示任意接口）
    my_addr.sin_port = htons(RECV_PORT);            // 接收端口（由 protocol.h 定义）
    if (bind(sd, (struct sockaddr *)&my_addr, sizeof(my_addr)) == -1)
    {
        perror("bind()");       // 绑定失败
        close(sd);              // 关闭套接字
        return -2;              // 退出，返回 -2
    }

    // [3] 加入到多播组（告诉内核接收指定多播地址的数据）
    inet_aton(MULTICAST_IP, &imr.imr_multiaddr);    // 多播组 IP（如 "234.2.3.4"）
    inet_aton(LOCAL_IP, &imr.imr_address);          // 本地接口 IP（"0.0.0.0"）
    imr.imr_ifindex = if_nametoindex(NETCARD_NAME); // 通过网卡名（如 "ens33"）获取网卡索引
    if (setsockopt(sd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
                   &imr, sizeof(imr)) == -1)        // 设置套接字选项：加入多播组
    {
        perror("setsockopt()"); // 加入失败
        close(sd);              // 关闭套接字
        return -3;              // 退出，返回 -3
    }

    // [4] 接收多播组中的消息（阻塞等待）
    // recvfrom 参数：sd, 缓冲区, 长度, 标志(0), 对端地址(NULL), 地址长度(NULL)
    recvfrom(sd, buf.msg, MSGSIZE, 0, NULL, NULL);
    puts(buf.msg);              // 将接收到的消息打印到标准输出（自动换行）

    // [5] 关闭套接字
    close(sd);

    return 0;   // 正常退出
}
