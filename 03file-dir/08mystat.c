/*获取文件的文件名*/
/*获取文件最后修改内容的时间*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>

#define BUFSIZE 10
#define TIMELEN 32

static char get_file_type(mode_t st_mode) // 获取文件的类型
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
    int mask[BUFSIZE - 1] = {S_IRUSR, S_IWUSR, S_IXUSR,
                             S_IRGRP, S_IWGRP, S_IXGRP,
                             S_IROTH, S_IWOTH, S_IXOTH}; // 存储权限的宏值
    char permission[BUFSIZE] = "rwxrwxrwx";              // 满权限
    int i = 0;                                           // 循环变量

    for (i = 0; i < BUFSIZE - 1; i++)
    {
        if (!(st_mode & mask[i])) // 判断当前文件是否没有该权限
            permission[i] = '-';  // 由于没有该权限,把该位置的符号修改为'-'
    }
    strncpy(buf, permission, BUFSIZE); // 把局部数组的数据转存到形参指向的存储空间

    return buf;
}

static int get_file_nlink(nlink_t st_nlink)
{
    return st_nlink;
}

static char *get_file_uname(uid_t st_uid)
{
    struct passwd *p = NULL; // 指针p指向获取到的用户信息

    p = getpwuid(st_uid); // 根据uid获取该用户的用户信息
    if (p == NULL)        // 判断获取用户信息是否失败
    {
        perror("getpwuid()"); // 打印错误信息
        return NULL;          // 由于获取用户信息失败,结束函数,并且返回NULL
    }
    return p->pw_name; // 返回用户信息中的user name
}

static char *get_file_gname(gid_t st_gid)
{
    struct group *p = NULL; // 指针p指向获取到的组信息

    p = getgrgid(st_gid); // 根据gid获取该组的组信息
    if (p == NULL)        // 判断获取组信息是否失败
    {
        perror("getgrgid()"); // 打印错误信息
        return NULL;          // 由于获取用户信息失败,结束函数,并且返回NULL
    }
    return p->gr_name; // 返回用户信息中的group name
}

static off_t get_file_size(off_t st_size)
{
    return st_size;
}

static char *get_file_mtime(time_t tm, char *tbuf)
{
    struct tm *p = NULL; // p指针指向转换后的时间结构体

    p = localtime(&tm); // 把时间戳转换成时间结构
    if (p == NULL)      // 判断转换是否失败
    {
        perror("localtime()"); // 打印错误信息
        return NULL;           // 由于转换失败,结束函数,并且返回NULL
    }
    strftime(tbuf, TIMELEN, "%m月 %d %H:%M", p); // 把时间结构转换为格式化时间的字符串

    return tbuf;
}

int main(int argc, char *argv[])
{
    struct stat fs;           // 用来存储获取到的文件元信息
    char buf[BUFSIZE] = {0};  // 用来存储文件权限的符号
    char tbuf[TIMELEN] = {0}; // 用来存储文件最后修改内容时间的字符串

    if (argc < 2) // 判断命令行参数的个数是否少于2个
    {
        fprintf(stderr, "Usage : %s + filename\n", argv[0]); // 打印使用说明
        return -1;                                           // 由于命令行参数的个数少于2个,结束程序,并且返回-1
    }

    if (stat(argv[1], &fs) == -1) // 判断获取文件的元信息是否失败
    {
        perror("stat()"); // 打印错误信息
        return -2;        // 由于获取文件的元信息失败,结束程序,并且返回-2
    }

    printf("%c", get_file_type(fs.st_mode));            // 获取文件的类型
    printf("%s", get_file_permission(fs.st_mode, buf)); // 获取文件的权限
    printf(" %d ", get_file_nlink(fs.st_nlink));        // 获取文件的硬链接数
    printf("%s", get_file_uname(fs.st_uid));            // 获取文件的所属者
    printf(" %s ", get_file_gname(fs.st_gid));          // 获取文件的所属组
    printf("%ld ", get_file_size(fs.st_size));          // 获取文件的字节大小
    printf("%s ", get_file_mtime(fs.st_mtime, tbuf));   // 获取文件的最后修改内容时间
    printf("%s\n", argv[1]);
    return 0;
}
