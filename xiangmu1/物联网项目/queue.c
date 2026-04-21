#include <stdio.h>      // 标准输入输出（未直接使用，但预留）
#include <stdlib.h>     // 标准库（malloc, calloc, free）
#include <string.h>     // 字符串操作（memcpy, memset）
#include "queue.h"      // 队列模块头文件

// 初始化环形队列
// 参数: q        - 输出参数，回填队列指针
//       capacity - 队列容量（最多存储 capacity 个元素）
//       size     - 每个元素的大小
// 返回值: 成功返回 0，失败返回 -1 或 -2
int queue_init(queue_t **q, int capacity, int size)
{
    // 为队列结构体分配内存
    *q = malloc(sizeof(queue_t));
    if(*q == NULL)              // 内存分配失败
        return -1;

    // 分配队列底层存储空间（环形缓冲区，容量+1 是为了区分空和满）
    (*q)->s = calloc(capacity + 1, size);
    if((*q)->s == NULL)         // 内存分配失败
    {
        free(*q);               // 释放已分配的队列结构体
        *q = NULL;              // 将指针置空
        return -2;              // 返回 -2 表示数据区分配失败
    }

    // 初始化队列成员
    (*q)->front = (*q)->rear = 0;       // 队头和队尾都指向 0（空队列）
    (*q)->capacity = capacity + 1;      // 实际容量 = 用户容量 + 1（用于区分空满）
    (*q)->size = size;                  // 元素大小

    return 0;   // 初始化成功
}

// 判断队列是否为空
// 参数: q - 队列指针
// 返回值: 空返回 1，非空返回 0
int queue_is_empty(const queue_t *q)
{
    return q->front == q->rear;     // 队头等于队尾时队列为空
}

// 判断队列是否为满
// 参数: q - 队列指针
// 返回值: 满返回 1，不满返回 0
int queue_is_full(const queue_t *q)
{
    // 队尾的下一个位置（环形）等于队头时，队列为满
    return (q->rear + 1) % q->capacity == q->front;
}

// 入队操作（在队尾添加元素）
// 参数: q    - 队列指针
//       data - 指向要添加的数据的指针
// 返回值: 成功返回 0，队列已满返回 -1
int queue_en(queue_t *q, const void *data)
{
    if(queue_is_full(q))            // 队列已满，无法入队
        return -1;

    // 移动队尾指针（环形移动）
    q->rear = (q->rear + 1) % q->capacity;
    // 将数据复制到队尾位置（地址 = 基址 + 索引 * 元素大小）
    memcpy((char *)q->s + q->rear * q->size, data, q->size);
    return 0;
}

// 出队操作（从队头取出元素）
// 参数: q    - 队列指针
//       data - 指向存储取出数据的缓冲区
// 返回值: 成功返回 0，队列为空返回 -1
int queue_de(queue_t *q, void *data)
{
    if(queue_is_empty(q))           // 队列为空，无法出队
        return -1;

    // 移动队头指针（环形移动）
    q->front = (q->front + 1) % q->capacity;
    // 将队头元素的数据复制到用户缓冲区
    memcpy(data, (char *)q->s + q->front * q->size, q->size);
    // 清空原位置数据（可选，防止残留）
    memset((char *)q->s + q->front * q->size, '\0', q->size);
    return 0;
}

// 销毁队列，释放所有动态分配的内存
// 参数: q - 指向要销毁的队列指针
// 返回值: 无
void queue_destroy(queue_t *q)
{
    free(q->s);     // 释放底层存储空间
    free(q);        // 释放队列结构体本身
}
