/*通过创建4个子进程实现计算100~300之间的质数
 * 【1】创建父子进程
 * 【2】使用子进程循环计算100~300之间的质数
 * 【3】父进程为子进程收尸
 * */

#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>

#define MIN	100
#define MAX 300

/*
 *
 *功能：进行是否是质数的功能
 *参数：num	获取数值
 *返回值：成功返回数值，失败返回0
 *
 * */
static int is_prime(int num)
{
	//定义变量循环计算
	int i = 0;

	//进行1s的睡眠
	sleep(1);

	//判断数值是否是1，如果是则不是质数
	if(num <= 1)
		return 0;
	//判断数值是否是2和3，如果是则返回1
	if(num == 2 || num == 3)
		return 1;
	//判断数值是否是质数
	for(i=2;i<=num/i;i++)
	{
		if(num%i==0)
			return 0;
	}

	return 1;
}

int main(void)
{
	//定义循环变量来创建子进程
	int i = 0;
	//定义存储子进程的标识
	pid_t pid;
	//定义循环变量遍历100~300之间的数字
	int n = 0;


	//循环创建子进程
	for(i=0;i<4;i++)
	{
		//创建子进程
		pid=fork();
		//判断创建子进程是否失败
		if(pid == -1)
		{
			//由于创建子进程失败，在终端打印错误信息
			perror("fork()");
			//由于创建子进程失败，终止程序
			exit(1);
		}

		//子进程的操作
		if(pid == 0)
			break;
	}


	//循环遍历100~300之间的数字，使用子进程进行计算并输出
	for(n=MIN;n<=MAX;n++)
	{
		//判断使用哪个子进程
		if(n%4==i)
		{
			//判断100~300之间的数字是否是质数
			if(is_prime(n))
				printf("[%d] %d Is A Prime Number!\n",i,n);
		}
	}


	//分支终止子进程
	switch(i)
	{
		case 0:
		case 1:
		case 2:
		case 3:exit(0);
	}


	//循环等待子进程结束，父进程为子进程收尸
	for(i=0;i<4;i++)
		wait(NULL);


	//正常结束程序
	return 0;
}

