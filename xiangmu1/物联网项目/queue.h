#ifndef __QUEUE_H               // 头文件保护宏，防止重复包含
#define __QUEUE_H

// 环形队列结构体（基于数组实现）
typedef struct
{
    void *s;            // 指向队列底层存储空间的指针（动态分配）
    int front;          // 队头索引（指向第一个有效元素的前一个位置）
    int rear;           // 队尾索引（指向最后一个有效元素）
    int capacity;       // 队列总容量（实际存储空间 = capacity * size，环形实现需预留一个空位）
    int size;           // 每个元素的大小（字节）
} queue_t;

/*
功能 : 队列初始化
参数 : q        - 输出参数，用于回填创建的队列指针
       capacity - 队列容量（最多可存储的元素个数）
       size     - 每个元素的大小（字节）
返回值 : 成功返回 0，失败返回 -1（内存分配失败）或 -2（数据区分配失败）
*/
int queue_init(queue_t **q, int capacity, int size);

/*
功能 : 判断队列是否为空
参数 : q - 指向队列结构体的指针
返回值 : 为空返回 1，不为空返回 0
*/
int queue_is_empty(const queue_t *q);

/*
功能 : 判断队列是否为满
参数 : q - 指向队列结构体的指针
返回值 : 为满返回 1，不为满返回 0
*/
int queue_is_full(const queue_t *q);

/*
功能 : 入队（将数据放入队列尾部）
参数 : q    - 指向队列结构体的指针
       data - 指向要放入的数据的指针
返回值 : 成功返回 0，失败（队列已满）返回 -1
*/
int queue_en(queue_t *q, const void *data);

/*
功能 : 出队（从队列头部取出数据）
参数 : q    - 指向队列结构体的指针
       data - 指向存储取出数据的缓冲区的指针
返回值 : 成功返回 0，失败（队列为空）返回 -1
*/
int queue_de(queue_t *q, void *data);

/*
功能 : 销毁队列，释放所有动态分配的内存
参数 : q - 指向要销毁的队列结构体的指针
返回值 : 无
*/
void queue_destroy(queue_t *q);

#endif   // __QUEUE_H 结束头文件保护
