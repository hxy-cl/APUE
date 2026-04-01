#include <stdio.h>

int main(void)
{
	FILE *fp = NULL;

	fp = fopen("abc","w");
	if(fp==NULL)
	{
		perror("fopen()");
		return -1;
	}

	printf("fp=%p\n",fp);

	fclose(fp);
	fp=NULL;

	return 0;
}

