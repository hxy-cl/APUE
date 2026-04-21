/*获取特定的环境变量*/
#include <stdio.h>
#include <stdlib.h>

int main(int argc,char *argv[])
{
	puts(getenv(argv[1]));

	return 0;
}
