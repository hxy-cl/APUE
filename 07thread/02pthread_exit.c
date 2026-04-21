/*创建线程并且终止线程*/

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * 功能：创建出来的线程要执行的函数
 *参数：void arg
 *返回值：空
 */
static void *thr_job(void *arg)
{
	//定义变量i初始化为0
	int i = 0;

	//循环次数为10
	for(i=0;i<10;i++)
	{
		//判断循环次数为5的时候，终止线程
		if(i == 5)
			//终止线程
			pthread_exit(NULL);
		//往标准输出中输出1byte !
		write(1,"!",1);
		//进行1s的睡眠
		sleep(1);
	}
}

int main(void)
{
	//定义存储新创建线程的标识
	pthread_t tid;
	//定义存储返回值
	int ret = 0;


	//创建线程
	ret=pthread_create(&tid,NULL,thr_job,NULL);
	//判断线程是否创建失败
	if(ret != 0)
	{
		//由于创建线程失败，在终端提示错误信息
		fprintf(stderr,"pthread_create():%s\n",strerror(ret));
		//由于创建线程失败，终止进程，并且设置状态1
		exit(1);
	}


	//死循环
	while(1)
	{
		//往标准输出中写入1byte *
		write(1,"*",1);
		//进行1s的睡眠
		sleep(1);
	}

	//程序正常结束
	return 0;
}
