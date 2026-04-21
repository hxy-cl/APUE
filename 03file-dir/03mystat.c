/*获取文件的硬链接数*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>

#define BUFSIZE 10

static char get_file_type(mode_t st_mode)
{
    char c = 0; // 用来存储解析出来的文件类型的符号

    switch (st_mode & S_IFMT)
    {
    case S_IFREG:
        c = '-';
        break;
    case S_IFDIR:
        c = 'd';
        break;
    case S_IFCHR:
        c = 'c';
        break;
    case S_IFBLK:
        c = 'b';
        break;
    case S_IFSOCK:
        c = 's';
        break;
    case S_IFIFO:
        c = 'p';
        break;
    case S_IFLNK:
        c = 'l';
        break;
    default:
        c = '?';
        break;
    }
    return c;
}

static char *get_file_permission(mode_t st_mode, char *buf)
{
    // 基本权限掩码（顺序：所有者、组、其他）
    int mask[BUFSIZE - 1] = {S_IRUSR, S_IWUSR, S_IXUSR,
                             S_IRGRP, S_IWGRP, S_IXGRP,
                             S_IROTH, S_IWOTH, S_IXOTH};
    char permission[BUFSIZE] = "rwxrwxrwx"; // 初始化为可读、可写、可执行字符串

    // 去掉没有的权限
    for (int i = 0; i < BUFSIZE - 1; i++)
    {
        if (!(st_mode & mask[i]))
            permission[i] = '-';
    }

    // 处理 Set-UID（所有者执行位）
    if (st_mode & S_ISUID)
    {
        permission[2] = (permission[2] == 'x') ? 's' : 'S';
    }
    // 处理 Set-GID（组执行位）
    if (st_mode & S_ISGID)
    {
        permission[5] = (permission[5] == 'x') ? 's' : 'S';
    }
    // 处理 Sticky Bit（其他用户执行位）
    if (st_mode & S_ISVTX)
    {
        permission[8] = (permission[8] == 'x') ? 't' : 'T';
    }

    strncpy(buf, permission, BUFSIZE); // 复制到输出缓冲区
    return buf;
}

static int get_file_nlink(nlink_t st_nlink)
{
    return st_nlink;
}

int main(int argc, char *argv[])
{
    struct stat fs;
    char buf[BUFSIZE] = {0};
    if (argc < 2)
    {
        fprintf(stderr, "Usage:%s+filename\n", argv[0]);
        return -1;
    }

    if (stat(argv[1], &fs) == -1)
    {
        perror("stat()");
        return -2;
    }

    printf("%c", get_file_type(fs.st_mode));
    printf("%s ", get_file_permission(fs.st_mode, buf));
    printf("%d \n", get_file_nlink(fs.st_nlink));
    return 0;
}