#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

int main(int argc,char *argv[])
{
	int fd = 0;
	int i = 0;

	if(argc<2)
	{
		fprintf(stderr,"Usage:%s+filename\n",argv[0]);
		return -1;
	}

	for(i=1;i<argc;i++)
	{
		fd = open(argv[i],O_WRONLY | O_CREAT,0666);
		if(fd<0)
		{
			perror("open()");
			return -2;
		}
		close(fd);
	}

	return 0;
}
