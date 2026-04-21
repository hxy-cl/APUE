/*一个进程有哪些环境变量*/
#include <stdio.h>

int main(int argc,char *argv[],char *env[])
{
	int i = 0;//定义一个变量，进行循环
			  
	printf("argc = %d\n",argc);

	for(i = 0;i<argc;i++)
		puts(argv[i]);
	printf("\n");

	for(i=0;env[i]!=NULL;i++)
		puts(env[i]);
	printf("\n");

	return 0;

}
