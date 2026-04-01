#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define BUFSIZE 128

static int mycat(int fd)
{
	char buf[BUFSIZE]={0};
	int count = 0;
	
	while(1)
	{
		memset(buf,0,BUFSIZE);
		count = read(fd,buf,BUFSIZE);
		if(count == 0)
		{
			break;
		}
		else if(count<0)
		{
			perror("read()");
			return -1;
		}
		write(1,buf,count);
	}
	return 0;
}
int main(int argc,char *argv[])
{
	int fd = 0;
	off_t count = 0;

	if(argc<2)
	{
		fprintf(stderr,"Usage:%s+filename\n",argv[0]);
		return -1;
	}
	
	fd=open(argv[1],O_RDONLY);
	if(fd<0)
	{
		perror("open");
		return -2;
	}
	
	count = lseek(fd,-64,SEEK_END);
	if(count<0)
	{
		perror("lseek()");
	}
	printf("count = %ld\n",count);

	mycat(fd);

	close(fd);
	
	return 0;
}
