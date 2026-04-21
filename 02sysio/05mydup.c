/*使用系统效用IO函数实现mycat*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define DUP2

#define BUFSIZE 128

static int mycat(int fd)
{
	char buf[BUFSIZE] = {0};
	int count = 0;

	while (1)
	{
		memset(buf, 0, BUFSIZE);
		count = read(fd, buf, BUFSIZE);
		if (count == 0)
			break;
		else if (count < 0)
		{
			perror("read()");
			return -1;
		}
		write(1, buf, count);
	}
}

int main(int argc, char *argv[])
{
	int fd = 0; // fd变量用来保存打开文件的文件描述法
	int fd0 = 0;

	if (argc < 3)
	{
		fprintf(stderr, "Usage:%s+srcname+destfile\n", argv[0]);
		return -1;
	}

	fd = open(argv[1], O_RDONLY);
	if (fd < 0)
	{
		perror("open()");
		return -2;
	}

	fd0 = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
	if (fd0 < 0)
	{
		perror("open()");
		return -3;
	}

#if defined(DUP)
	// 不是原子操作
	close(1); // 关闭1号文件描述符
	dup(fdo); // 把当前可用最小的文件描述符(1)号作为fd0的复制
#elif defined(DUP2)
	// 原子操作
	dup2(fd0, 1); // 手动指定1号文件描述符作为fd0的复制
#endif
	mycat(fd);

	close(fd);
	close(fd0);

	return 0;
}
