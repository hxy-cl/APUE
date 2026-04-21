/*修改或者添加一个环境变量*/
#include <stdio.h>
#include <stdlib.h>

int main(int argc,char *argv[])
{
	setenv("TEST","ARE YOU OK?",1);//添加一个环境变量
	puts(getenv("TEST"));
	return 0;
}
