#include <stdio.h>
#include <string.h>

#define FGETS
#define BUFSIZE 1024

static int mycat(const char *pathname)
{
	FILE *fp = NULL;
	int ch = 0;
	char buf[BUFSIZE];

	fp=fopen(pathname,"r");
	if(fp==NULL)
	{
		perror("fopen()");
		return -1;
	}

	while(1)
	{
#if defined(FGETC)
		ch = fgetc(fp);
		if(ch==EOF)
		{
			if(ferror(fp))
			{
				fclose(fp);
				return -2;
			}
			break;
		}
		fputc(ch,stdout);
#elif defined(FGETS)
		memset(buf,0,BUFSIZE);
		if(fgets(buf,BUFSIZE,fp)==NULL)
			break;
		fputs(buf,stdout);
#endif
	}
	fclose(fp);
	return 0;
}

int main(int argc,char *argv[])
{
	if(argc<2)
	{
		fprintf(stderr,"Usage:%s+filename\n",argv[0]);
		return -1;
	}

	mycat(argv[1]);

	return 0;
}
