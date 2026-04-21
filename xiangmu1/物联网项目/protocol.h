#ifndef __PROTOCOL_H            // 头文件保护宏，防止重复包含
#define __PROTOCOL_H

// [1] 多播组的 IP 地址（用于 UDP 多播通信）
#define MULTICAST_IP "234.2.3.4"

// [2] 本地 IP 地址，绑定多播时使用 "0.0.0.0" 表示任意网络接口
#define LOCAL_IP "0.0.0.0"

// [3] 当前系统的网卡名称（用于设置多播接口，需根据实际环境修改，如 eth0）
#define NETCARD_NAME "ens33"

// 服务器端 TCP 监听 IP 地址（用于 TCP 服务）
#define SERVER_IP "10.11.17.88"

// 通信消息的最大长度（字节）
#define MSGSIZE 128

// TCP 服务监听端口
#define TCP_PORT        8888

// UDP 服务监听端口
#define UDP_PORT        8889

// 心跳检测间隔（秒）
#define HEARTBEAT_INTERVAL 30

// 客户端认证令牌（TCP 连接建立后必须先发送此字符串进行认证）
#define AUTH_TOKEN "iot_gateway_2024"

// 通信协议数据包结构（用于 TCP 自定义协议）
// 使用 __attribute__((packed)) 强制单字节对齐，避免结构体填充
struct data_st
{
    int8_t id;                  // 消息类型 ID（有符号8位整数）
    char msg[MSGSIZE];          // 消息内容（固定长度）
} __attribute__((packed));      // 单字节对齐，保证网络传输无额外字节

#endif   // __PROTOCOL_H 结束头文件保护
