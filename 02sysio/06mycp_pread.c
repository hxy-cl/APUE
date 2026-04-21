/*使用原子操作pread和pwrite实现mycp的功能
 *使用原子操作的原因：避免竞态的问题*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#define BUFSIZE 128

static int mycp(int fds, int fdd)
{
    char buf[BUFSIZE] = {0};
    int count = 0;
    off_t offset = 0;

    while (1)
    {
        memset(buf, 0, BUFSIZE);
        count = pread(fds, buf, BUFSIZE, offset);
        if (count == 0)
            break;
        else if (count < 0)
        {
            perror("pread()");
            return -1;
        }
        pwrite(fdd, buf, count, offset);
        offset += count;
    }
}

int main(int argc, char *argv[])
{
    int fds = 0;
    int fdd = 0;

    // 判断命令行参数是否少于3个
    if (argc < 3)
    {
        // 由于命令行参数少于3个，打印错误信息到标准输出中
        fprintf(stderr, "Usage:%s+srcfile+destfile\n", argv[0]);
        // 由于命令行参数少于3个，结束函数，并且返回-1
        return -1;
    }

    fds = open(argv[1], O_RDONLY);
    if (fds < 0)
    {
        perror("open()");
        return -2;
    }

    fdd = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fdd < 0)
    {
        close(fds);
        perror("open()");
        return -3;
    }

    mycp(fds, fdd);

    close(fdd);
    close(fds);

    return 0;
}