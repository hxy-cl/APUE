/*使用信号量数组实现创建4个子进程，让子进程去读写文件，实现mycat功能
 *【1】打开文件
 *【2】创建信号量
 *【3】信号量操作
 *【4】父子进程操作、读写文件数据
 *【5】终止进程、销毁信号量
 *【6】关闭文件
 * */

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

//定义字节大小
#define BUFSIZE 8

//定义信号量专用的共用体
union semun
{
	//用于存储信号量的初始值
	int val;
};

int main(int argc,char *argv[])
{
	//fd变量用来保存打开文件的文件描述符
	int fd = 0;
	//定义循环变量
	int i = 0;
	//定义存储子进程的标识
	pid_t pid;
	//定义存储从文件中读取的数据
	char buf[BUFSIZE] = {0};
	//用来存储成功读取到的字节数
	ssize_t count = 0;
	//定义信号量P操作的结构体变量
	struct sembuf p_op = {0,-1,SEM_UNDO};
	//定义信号量V操作的结构体变量
	struct sembuf v_op = {0,1,SEM_UNDO};
	//定义存储信号量的标识
	int semid = 0;
	//定义信号量初始化的共用体变量
	union semun tmp;


	//判断命令行参数个数是否少于2个
	if(argc<2)
	{
		//在终端提示错误信息
		fprintf(stderr,"Usage:%s+filaname!\n",argv[0]);
		//由于命令行参数个数少于2个，结束程序并且返回-1
		return -1;
	}
	

	//通过open(2)以只读的形式打开文件
	fd = open(argv[1],O_RDONLY);
	//判断打开文件是否失败
	if(fd<0)
	{
		//在终端提示打开文件失败
		perror("open");
		//由于打开文件失败，结束程序并且返回-2
		return -2;
	}


	//创建信号量，使用亲缘关系，创建并且设置权限为所属者读写，其他无权限
	semid=semget(IPC_PRIVATE,1,IPC_CREAT | 0600);
	//将信号量的初始值设置为1(互斥锁)
	tmp.val = 1;
	//把信号量数组下标为0的信号量初始化
	semctl(semid,0,SETVAL,tmp);


	//循环创建子进程
	for(i=0;i<4;i++)
	{
		//创建子进程
		pid = fork();
		//判断创建子进程是否失败
		if(pid == -1)
		{
			//由于创建子进程失败，在终端提示错误信息
			perror("fork()");
			//由于创建子进程失败，关闭文件描述符
			close(fd);
			//由于创建子进程失败，终止程序
			exit(1);
		}

		//子进程创建成功，进行子进程的操作
		if(pid == 0)
			break;
	}


	while(1)
	{
		//进行P操作，加锁
		semop(semid,&p_op,1);
		//清空脏数据
		memset(buf,0,BUFSIZE);
		//进行文件读数据
		count = read(fd,buf,BUFSIZE);
		//判断读取文件数据是否失败
		if(count == -1)
		{
			//读取文件数据失败，在终端打印错误信息
			perror("read");
			//进行信号量V操作，解锁
			semop(semid,&v_op,1);
			//由于读文件数据失败，终止程序
			exit(1);
		}

		//判断文件是否读到末尾
		if(count == 0)
		{
			//进行信号量V操作，解锁
			semop(semid,&v_op,1);
			//跳出死循环
			break;
		}

		//进行1s的睡眠
		sleep(1);
		//进行写数据，把数据从buf中写到终端中，1就是终端(stdout)
		write(1,buf,count);
		//进行信号量V操作，解锁
		semop(semid,&v_op,1);
	}


	//终止子进程
	switch(i)
	{
		case 0:
		case 1:
		case 2:
		case 3:exit(0);//正常退出程序
	}

	//等待子进程结束，父进程给子进程收尸
	for(i=0;i<4;i++)
		wait(NULL);

	//销毁信号量
	semctl(semid,0,IPC_RMID);

	//通过close(2)关闭文件
	close(fd);

	//程序正常结束
	return 0;
}
