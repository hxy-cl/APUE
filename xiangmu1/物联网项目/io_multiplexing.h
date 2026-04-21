#ifndef __IO_MULTIPLEXING_H   // 头文件保护宏，防止重复包含
#define __IO_MULTIPLEXING_H

// 向 poll 监听集合中添加一个新的文件描述符
// 参数: fd - 要添加的文件描述符（如 socket）
//       events - 感兴趣的事件（如 POLLIN、POLLOUT）
// 返回值: 无
void poll_add_fd(int fd, int events);

// 从 poll 监听集合中删除一个文件描述符，并关闭它
// 参数: fd - 要删除的文件描述符
// 返回值: 无
void poll_del_fd(int fd);

// 进入主事件循环（永不返回，直到程序退出）
// 参数: 无
// 返回值: 无
void poll_loop(void);

#endif   // __IO_MULTIPLEXING_H 结束头文件保护
