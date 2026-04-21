/*终止进程的方式*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int main(void)
{
    printf("Hello World!");
    /*
        return 0;
        内核->C启动历程->main()->printf()->main()
        ->return->C启动历程->exit(3)->标准IO清理程序->exit(3)
        ->_exit(2)或者_Exit(2)->内核
    */
    /*
        exit(0);
        内核->C启动历程->main()->printf()->main()
        ->exit(3)->标准IO清理程序->exit(3)
        ->_exit(2)或者_Exit(2)->内核
    */
    /*
        _exit(2);
        内核->C启动历程->main()->printf()->main()
        ->_exit(2)或者_Exit(2)->内核
    */
    /*
        如果main()最后什么也没有写,等同于return
    */
}
