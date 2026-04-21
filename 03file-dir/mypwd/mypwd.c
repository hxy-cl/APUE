/*密码的校验过程*/
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <crypt.h>
#include <shadow.h>

#define NAMESIZE 32

int main(void)
{
    char name[NAMESIZE] = {0};
    char *pwd = NULL;
    char *cp = NULL;
    struct spwd *tp = NULL;

    //[1]输入登录的用户名
    printf("请输入用户名：");
    fgets(name, NAMESIZE, stdin);
    *strchr(name, '\n') = '\0';

    //[2]输入密码
    pwd = getpass("请输入密码：");
    if (pwd == NULL)
    {
        perror("getpass");
        return -1;
    }

    //[3]读真正的密码
    tp = getspnam(name);
    if (tp == NULL)
    {
        fprintf(stderr, "获取shadow文件中的用户信息失败!\n");
        return -2;
    }

    //[4]将输入的密码进行加密
    cp = crypt(pwd, tp->sp_pwdp);
    if (cp == NULL)
    {
        perror("crypt()");
        return -3;
    }

    //[5]对比密码
    if (!strcmp(tp->sp_pwdp, cp))
        printf("恭喜!登录成功!\n");
    else
        printf("对不起,密码错误!\n");

    return 0;
}