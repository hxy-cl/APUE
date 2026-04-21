/*获取文件的类型*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

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

int main(int argc, char *argv[])
{
    struct stat fs; // 用来存储获取到的文件元信息

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

    printf("%c\n", get_file_type(fs.st_mode));

    return 0;
}
