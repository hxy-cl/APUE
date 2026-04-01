#include <stdio.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>

#define MIN 100
#define MAX 300

#define NUM (MAX - MIN + 1)

//semctl(2)在初始化信号量数组时必须使用的共用体(联合)
union semun
{   
    int val;//信号量的初始值
};

//全局变量(父子进程共享)
int *shm_ptr = NULL;//指向共享内存,访问指向空间得到的是要处理的数字
int semid;//信号量标识符

//功能 : 将信号量初始化为1(用作互斥锁)
void sem_init(int semid)
{
    union semun tmp;//定义共用体变量
    tmp.val = 1;//信号量初始值为1
    semctl(semid, 0, SETVAL, tmp);//对信号量数组中下标为0的信号量设置为1
}

//功能 : P操作->申请锁(信号量 -1,如果信号量为0,则阻塞等待)
void sem_p(int semid)
{
    struct sembuf buf;//定义信号量操作的结构体变量

    buf.sem_num = 0;//操作信号量数组中下标为 0 的信号量
    buf.sem_op = -1;//P操作 : 信号量 -1
    buf.sem_flg = SEM_UNDO;//进程退出时自动还原信号量

    semop(semid, &buf, 1);//执行信号量操作
}

//功能 : V操作->释放锁(信号量 +1,唤醒等待的进程)
void sem_v(int semid)
{
    struct sembuf buf;//定义信号量操作的结构体变量

    buf.sem_num = 0;//操作信号量数组中下标为 0 的信号量
    buf.sem_op = 1;//V操作 : 信号量 +1
    buf.sem_flg = SEM_UNDO;//进程退出时自动还原信号量

    semop(semid, &buf, 1);//执行信号量操作
}

static int is_prime(int num)
{
    int i = 0;//循环变量

    sleep(1);

    if(num <= 1)//判断num是否小于等于1(是否不是质数)
        return 0;
    if(num == 2 || num == 3)
        return 1;

    for(i = 2; i <= num / i; i++)
    {
        if(num % i == 0)
            return 0;
    }
    return 1;
}

void work(int n)//n作为子进程的编号
{
    int num = 0;//存储临界资源

    while(1)//子进程循环抢任务
    {
        sem_p(semid);//P操作:加锁
        if(*shm_ptr > MAX)//如果数字大于MAX,任务结束
        {
            sem_v(semid);//V操作:解锁
            break;//跳出死循环
        }
        num = *shm_ptr;//把临界资源拷贝出来
        (*shm_ptr)++;//临界资源自增
        sem_v(semid);//V操作:解锁
        if(is_prime(num))//判断是否是质数
            printf("[%d] %d Is A Prime Number!\n", n, num);
    }
}

int main(void)
{
    int i = 0;//循环变量
    pid_t pid;//存储子进程的标识
    int n = 0;//循环变量
    int shmid = 0;//用来存储共享内存的标识

    //[1]创建共享内存,大小为4byte即可,(可以容纳下一个整型数)
    shmid = shmget(IPC_PRIVATE, 4, IPC_CREAT | 0600);//创建共享内存
    shm_ptr = shmat(shmid, NULL, 0);//将共享内存映射到进程的虚拟地址空间
    *shm_ptr = MIN;//将最小值投放到共享内存中

    //[2]创建信号量数组,1个信号量
    semid = semget(IPC_PRIVATE, 1, IPC_CREAT | 0600);
    sem_init(semid);//调用内部函数进行初始化

    //[3]创建4个子进程
    for(n = 0; n < 4; n++)
    {
        pid = fork();//创建子进程
        if(pid < 0)//判断创建子进程是否失败
        {
            perror("fork()");//打印错误信息
            exit(1);//由于创建子进程失败,终止进程,并且返回状态1
        }
        if(pid == 0)//子进程的操作
        {
            work(n);//子进程执行work()->抢任务
            exit(0);//终止子进程
        }
    }

    //父进程的操作
    for(i = 0; i < 4; i++)
        wait(NULL);//等待子进程结束

    //[4]销毁
    shmdt(shm_ptr);//解除共享内存映射
    shmctl(shmid, IPC_RMID, NULL);//销毁共享内存
    semctl(semid, 0, IPC_RMID);//销毁信号量

    return 0;
}


