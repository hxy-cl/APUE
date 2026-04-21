#include <stdio.h>
#include <string.h>

int main(void)
{
    char buf[] = "Hello        World!"; // 存储要分割的字符串
    char *str = buf;                    // 使用str指针代替buf
    char *ch = " ";                     // 存储要分割的符号
    char *p = NULL;                     // 接收解析出来的子串

    while (1)
    {
        // p = strsep(&str, ch);//通过strsep(3)分割字符串
        p = strtok(str, ch); // 通过strtok(3)分割字符串
        if (p == NULL)       // 判断是否分割完毕
            break;           // 由于分割完毕,跳出死循环
        printf("%s\n", p);   // 打印分割出的子串
        str = NULL;
    }

    printf("buf = %s\n", buf);

    return 0;
}
