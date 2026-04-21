#include <stdio.h>      // 标准输入输出（perror）
#include <stdlib.h>     // 标准库（malloc, free）
#include <string.h>     // 字符串操作（memset）
#include <unistd.h>     // POSIX 系统调用（read, write）
#include <fcntl.h>      // 文件控制（fcntl, O_NONBLOCK）
#include <errno.h>      // 错误码（errno, EAGAIN）

#include "poll.h"       // 包含本模块的头文件

// 初始化有限状态机
// 参数: f   - 输出参数，指向状态机指针的指针
//       rfd - 读文件描述符
//       wfd - 写文件描述符
// 返回值: 成功返回 0，失败返回 -1（内存分配失败）
int fsm_init(fsm_t **f, int rfd, int wfd)
{   
    fsm_t *me = NULL;           // 指向新分配的状态机内存
    int saver = 0, savew = 0;   // 分别存储读/写文件的原有文件状态标志

    // 为状态机结构体分配内存
    me = malloc(sizeof(fsm_t));
    if(me == NULL)              // 内存分配失败
        return -1;              // 返回 -1 表示失败

    // 存储客户传递的文件描述符
    me->rfd = rfd;              // 读文件描述符
    me->wfd = wfd;              // 写文件描述符
    // 清空缓冲区（避免残留数据）
    memset(me->buf, 0, BUFSIZE);
    me->count = 0;              // 已读取字节数清零
    me->pos = 0;                // 已写入字节数清零
    me->state = STATE_R;        // 初始状态为读状态
    me->errmsg = NULL;          // 错误函数名指针初始化为 NULL

    // 获取读文件的当前文件状态标志
    saver = fcntl(me->rfd, F_GETFL);
    // 在读文件原有标志上添加非阻塞标志（O_NONBLOCK）
    fcntl(me->rfd, F_SETFL, saver | O_NONBLOCK);

    // 获取写文件的当前文件状态标志
    savew = fcntl(me->wfd, F_GETFL);
    // 在写文件原有标志上添加非阻塞标志
    fcntl(me->wfd, F_SETFL, savew | O_NONBLOCK);

    *f = me;                    // 将创建的状态机地址回填给调用者

    return 0;                   // 初始化成功
}

// 推动有限状态机执行一步
// 参数: f - 指向状态机的指针
// 返回值: 始终返回 0（当前实现未使用返回值）
int fsm_drive(fsm_t *f)
{   
    int ret = 0;                // 存储本次 write 实际写入的字节数

    // 根据当前状态进行不同处理
    switch(f->state)
    {
        case STATE_R :          // 读状态
            // 从读文件描述符读取最多 BUFSIZE 字节到缓冲区
            f->count = read(f->rfd, f->buf, BUFSIZE);
            if(f->count == -1)  // read 调用出错
            {
                // 检查是否为假错（EAGAIN/EWOULDBLOCK），非假错才是真正的错误
                if(errno != EAGAIN)
                {   
                    f->errmsg = "read()";   // 记录出错的函数名
                    f->state = STATE_E;     // 切换到错误状态
                }
                // 如果是 EAGAIN，则保持 R 状态，下次再次尝试
            }
            else if(f->count == 0)          // 读到文件末尾（EOF）
                f->state = STATE_T;         // 切换到终止状态
            else                            // 读取成功，有数据
            {   
                f->pos = 0;                 // 重置已写入位置为 0
                f->state = STATE_W;         // 切换到写状态
            }
            break;

        case STATE_W :          // 写状态
            // 尝试将缓冲区中未写入的数据写入写文件描述符
            ret = write(f->wfd, f->buf + f->pos, f->count);
            if(ret == -1)                   // write 调用出错
            {
                f->errmsg = "write()";      // 记录出错函数名
                f->state = STATE_E;         // 切换到错误状态
            }
            else
            {
                if(ret < f->count)          // 未写完所有数据（非阻塞写可能只写了部分）
                {
                    f->pos += ret;          // 累加已写入字节数
                    f->count -= ret;        // 剩余待写入字节数减少
                    // 状态保持为 W，下次继续写
                }
                else                        // 全部写完
                {
                    f->state = STATE_R;     // 切换回读状态，准备读下一批数据
                }
            }
            break;

        case STATE_E :          // 错误状态
            perror(f->errmsg);  // 打印错误信息（根据 errno 输出具体原因）
            f->state = STATE_T; // 切换到终止状态
            break;

        case STATE_T :          // 终止状态
            // 当前什么都不做，由调用者决定是否结束进程或清理资源
            break;

        default :               // 非法状态（防御性编程）
            break;
    }

    return 0;   // 始终返回 0
}

// 销毁有限状态机，释放内存
// 参数: f - 指向要销毁的状态机
// 返回值: 成功返回 0
int fsm_destroy(fsm_t *f)
{
    free(f);        // 释放状态机结构体占用的内存
    f = NULL;       // 将局部指针置为 NULL（注意：不影响调用者的指针，调用者需自己置 NULL）
    return 0;
}
