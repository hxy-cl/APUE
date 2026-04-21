/*使用多线程实现判断100~300之间的质数*/
/*缺点：创建线程太多，浪费资源*/

#include <stdio.h>
#include <pthread.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>

#define MIN 100
#define MAX 300

#define NUM	(MAX-MIN)+1

/*
 * 功能：判断质数
 * 参数：num	传入的数值
 * 返回值：返回计算完的数值(质数)*/
static int is_prime(int num)
{
	//定义循环变量i
	int i = 0;

	//判断num是否小于等于1(是否不是质数)
	if(num <= 1)
		return 0;
	
	//判断num是否等于2或者3
	if(num == 2 || num == 3)
		return 1;

	//循环计算判断质数
	for(i=2;i<=num/i;i++)
		if(num%i == 0)
			return 0;
	return 1;
}


/*
 * 功能：创建出来的线程要执行的函数
 * 参数：arg	传入的值
 * 返回值：空*/
void *thr_job(void *arg)
{
	//定义数据类型接收并强制类型转换
	int p = (int)arg;

	//判断是否是质数
	if(is_prime(p))
		printf("[%d] Is A Prime Number!\n",p);
}

int main(void)
{
	//存储NUM个线程的标识
	pthread_t tid[NUM];
	//定义循环变量
	int i = 0,j = 0;
	//定义用来存储的返回值
	int ret = 0;

	//循环创建多线程
	for(i = MIN,j = 0;i<= MAX;i++,j++)
	{
		//创建线程
		ret = pthread_create(tid+j,NULL,thr_job,(void *)i);
		//判断创建线程是否失败
		if(ret != 0)
		{
			//由于创建线程失败，在标准输出中打印错误信息
			fprintf(stderr,"pthread_create():%s\n",strerror(ret));
			//由于创建线程失败，终止线程，并且设置状态为
			exit(1);
		}
	}


	//循环等待线程完成并且为线程收尸
	for(i=0;i<NUM;i++)
		//线程收尸
		pthread_join(tid[i],NULL);

	//正常结束程序
	return 0;
}
