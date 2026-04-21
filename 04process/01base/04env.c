/*由于移植性的问题，使用全局的环境变量*/
#include <stdio.h>

extern char **environ;

int main(int argc,char *argv[])
{
	int i = 0;

	printf("argc=%d\n",argc);

	for(i=0;i<argc;i++)
		puts(argv[i]);
	printf("\n");

	for(i=0;environ[i]!=NULL;i++)
		puts(environ[i]);
	printf("\n");

	return 0;
}
