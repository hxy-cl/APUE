#include <stdio.h>
#include <string.h>

#define FGETS
#define BUFSIZE	1024

static int mycp(const char *srcfile,const char *destfile)
{
	FILE *fps = NULL;
	FILE *fpd = NULL;
	int ch = 0;
	char buf[BUFSIZE]={0};

	fps=fopen(srcfile,"r");
	if(fps==NULL)
	{
		perror("fopen()");
		return -1;
	}

	fpd=fopen(destfile,"w");
	if(fpd==NULL)
	{
		perror("fopen()");
		return -2;
	}

	while(1)
	{
#if defined(FGETC)
		ch = fgetc(fps);
		if(ch==EOF)
		{
			if(ferror(fps))
			{	
				fclose(fpd);
				fclose(fps);
				return -3;
			}
			break;
		}
		fputc(ch,fpd);
#elif defined(FGETS)
		memset(buf,0,BUFSIZE);
		if(fgets(buf,BUFSIZE,fps)==NULL)
			break;
		fputs(buf,fpd);
#endif
	}
	fclose(fpd);
	fclose(fps);
	return 0;
}

int main(int argc,char *argv[])
{
	if(argc<3)
	{
		fprintf(stderr,"Usage:%s+srcfile+destfile\n",argv[0]);
		return -1;
	}

	mycp(argv[1],argv[2]);

	return 0;
}
