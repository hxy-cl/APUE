#include <stdio.h>

int mytouch(const char *pathname)
{
	FILE *fp=NULL;

	fp=fopen(pathname,"a+");

	if(fp==NULL)
	{
		perror("fopen()");
		return -1;
	}

	fclose(fp);

	return 0;
}

int main(int argc,char *argv[])
{
	int i = 0;

	if(argc<2)
	{
		fprintf(stderr,"Usage:%s+filename\n",argv[0]);
		return -1;
	}

	for(i=1;i<argc;i++)
	{
		mytouch(argv[i]);
	}
	
	return 0;
}

