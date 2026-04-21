/*注册终止处理函数*/
#include <stdio.h>
#include <stdlib.h>

void close_file(void)
{
	printf("善后【1】：关闭打开文件！\n");
}

void free_mem(void)
{
	printf("善后【2】：释放开辟的空间!\n");
}

void exit_begin(void)
{
	printf("善后【3】：开始进行终止处理程序!\n");
}

int main(void)
{
    if (atexit(close_file) != 0)
    {
        fprintf(stderr,"register close_file() is failed!\n");
		exit(1);
    }

	if(atexit(free_mem)!=0)//判断注册free_mem()是否失败
	{
		fprintf(stderr,"register free_mem is failed!\n");
		exit(1);//由于注册free_mem()失败，终止进程并返回状态1
	}

	if(atexit(exit_begin)!=0)
	{
		fprintf(stderr,"register exit_begin is failed!\n");
		exit(1);
	}

	printf("整个程序的业务逻辑，执行中...\n");

	exit(0);//也可以替换为return 0;
}
