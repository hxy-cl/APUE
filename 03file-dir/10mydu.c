/*实现mydu的命令*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <dirent.h>
#include <string.h>

#define BUFSIZE 1024

static long _mydu_blocks(const char *pathname)
{
    struct stat fs;
    long count = 0;
    DIR *dp = NULL;
    struct dirent *entry = NULL;
    char buf[BUFSIZE] = {0};

    if (stat(pathname, &fs) == -1)
    {
        perror("stat()");
        return -1;
    }

    if (!S_ISDIR(fs.st_mode))
    {
        return fs.st_blocks; // 直接返回非目录文件的块数
    }

    count = fs.st_blocks; // 获取目录本身的块数

    dp = opendir(pathname);
    if (dp == NULL)
    {
        perror("opendir()");
        return -2;
    }

    while (1)
    {
        errno = 0;
        entry = readdir(dp);
        if (entry == NULL)
        {
            if (errno != 0)
            {
                closedir(dp);
                perror("readdir()");
                return -3;
            }
            break;
        }
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, ".."))
            continue;               // 跳过目录中的.和..目录
        memset(buf, 0, BUFSIZE);    // 清空buf空间
        strcpy(buf, pathname);      // 先把当前操作的目录拷贝到buf空间中
        strcat(buf, "/");           // 拼接目录下的符号
        strcat(buf, entry->d_name); // 拼接子文件名
        count += _mydu_blocks(buf); // 递归累加子文件的块数
    }
    return count;
}
static long mydu(const char *pathname) // 把获取到的所占磁盘空间大小进行单位转换
{
    return _mydu_blocks(pathname) / 2; // 所占磁盘空间的块/2=Kb
}
int main(int argc, char *argv[])
{

    if (argc < 2)
    {
        fprintf(stderr, "Usage:%s+filename!\n", argv[0]);
        return -1;
    }

    printf("%ldK\t%s\n", mydu(argv[1]), argv[1]); // 打印argv[1]文件所占磁盘空间的大小

    return 0;
}