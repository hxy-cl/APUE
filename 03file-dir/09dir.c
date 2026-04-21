/*读取目录*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>

int main(int argc, char *argv[])
{
    struct stat fs;              // 用来存储获取到的文件元信息
    int ret = 0;                 // 用来存储错误码
    struct dirent *entry = NULL; // entry指针指向目录项结构
    DIR *dp = NULL;              // dp指针指向打开的目录流

    if (argc < 2)
    {
        fprintf(stderr, "Usage:%s+filename\n", argv[0]);
        ret = -1;
        goto ERR_1;
    }

    if (stat(argv[1], &fs) == -1) // 判断获取文件元信息是否失败
    {
        perror("stat()");
        ret = -2;
        goto ERR_1;
    }

    if (!S_ISDIR(fs.st_mode))
    {
        printf("%s NOT A Directory!\n", argv[1]);
        ret = -3;
        goto ERR_1;
    }

    dp = opendir(argv[1]);
    if (dp == NULL)
    {
        perror("opendir()");
        ret = -4;
        goto ERR_2;
    }

    while (1)
    {
        errno = 0; // 为了防止errno存储之前的错误码，进行清空处理
        entry = readdir(dp);
        if (entry == NULL) // 判断是否读取结束或者读取失败
        {
            if (entry != 0) // 判断是否读取失败
            {
                perror("readdir()");
                ret = -5;
                goto ERR_2;
            }
            break;
        }
        printf("%ld-%s\n", entry->d_ino, entry->d_name); // 打印子目录的inode号以及文件名
    }
ERR_2:
    closedir(dp); // 关闭目录流
ERR_1:
    return ret;
}