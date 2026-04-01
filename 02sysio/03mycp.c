#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define BUFSIZE 128

static int mycp(int fds,int fdd)
{
	char buf[BUFSIZE]={0};
	int count = 0;

	while(1)
	{
		memset(buf,0,BUFSIZE);
		count = read(fds,buf,BUFSIZE);
		if(count==0)
			break;
		else if(count <0)
		{
			perror("read()");
			return -1;
		}
		write(fdd,buf,count);
	}
	return 0;
}

int main(int argc,char *argv[])
{
	int fds = 0;
	int fdd = 0;

	if(argc<3)
	{
		fprintf(stderr,"Usage:%s+srcfile+destfile\n",argv[0]);
		return -1;
	}
	
	fds=open(argv[1],O_RDONLY);
	if(fds<0)
	{
		perror("open()");
		return -2;
	}

	fdd=open(argv[2],O_WRONLY|O_CREAT|O_TRUNC,0666);
	if(fdd<0)
	{
		close(fds);
		perror("open()");
		return -3;
	}
	
	mycp(fds,fdd);

	close(fdd);
	close(fds);

	return 0;
}
