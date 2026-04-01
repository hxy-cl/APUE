#include <stdio.h>
#include <unistd.h>

int main(void)
{
	int i = 0;

	for(i=0;i<5;i++)
	{
		printf("%d ",i);
		sleep(1);
	}

	return 0;
}
