#include <stdio.h>          // 标准输入输出（perror, printf）
#include <sys/types.h>      // 基本系统数据类型
#include <sys/socket.h>     // socket 编程接口
#include <stdlib.h>         // 标准库（exit）
#include <string.h>         // 字符串操作
#include <netinet/in.h>     // 网络地址结构（sockaddr_in, IPPROTO_IP）
#include <netinet/ip.h>     // IP 协议相关
#include <arpa/inet.h>      // 地址转换函数（inet_aton）
#include <unistd.h>         // 系统调用（close, sleep）
#include <net/if.h>         // 网络接口操作（if_nametoindex）

#include "protocol.h"       // 项目协议头文件（定义 MULTICAST_IP, LOCAL_IP, RECV_PORT, data_st 等）

int main(void)
{
    int sd = 0;                     // 套接字文件描述符
    struct data_st buf;             // 存储要发送的数据（本例未实际使用该变量）
    struct sockaddr_in remote_addr; // 对端（多播组）地址结构
    struct ip_mreqn imr;            // 多播组选项结构（用于设置发送接口）

    // [1] 创建 UDP 套接字（IPv4，数据报）
    sd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sd == -1)                   // 创建失败
    {
        perror("socket()");
        return -1;                  // 退出，返回 -1
    }

    // 注意：此处没有 bind，套接字的本地地址由内核自动分配（端口随机）

    // [2] 设置多播发送接口（指定从哪个网卡发出多播数据）
    inet_aton(MULTICAST_IP, &imr.imr_multiaddr);    // 多播组 IP（如 "234.2.3.4"）
    inet_aton(LOCAL_IP, &imr.imr_address);          // 本地接口 IP（"0.0.0.0" 表示内核选择）
    imr.imr_ifindex = if_nametoindex(NETCARD_NAME); // 通过网卡名获取索引（如 "ens33"）
    if (setsockopt(sd, IPPROTO_IP, IP_MULTICAST_IF,
                   &imr, sizeof(imr)) == -1)        // 设置多播发送接口选项
    {
        perror("setsockopt()");
        close(sd);
        return -2;                  // 退出，返回 -2
    }

    // [3] 往多播组中循环发送消息
    remote_addr.sin_family = AF_INET;               // IPv4
    inet_aton(MULTICAST_IP, &remote_addr.sin_addr); // 目标地址为多播组地址
    remote_addr.sin_port = htons(RECV_PORT);        // 目标端口（接收方需要监听此端口）

    while (1)   // 无限循环，每秒发送一次 "hello"
    {
        // 发送数据报（内容 "hello"，长度 5 字节，标志 0）
        sendto(sd, "hello", 5, 0,
               (struct sockaddr *)&remote_addr, sizeof(remote_addr));
        sleep(1);   // 休眠 1 秒
    }

    // [4] 关闭套接字（永远不会执行到这里）
    close(sd);
    return 0;
}
