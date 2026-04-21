/*跨函数的使用*/
#include <stdio.h>
#include <setjmp.h>

jmp_buf env;

int div(int num1, int num2)
{
    if (num2 == 0)
        longjmp(env, 1);
    return num1 / num2;
}

int main(void)
{
    int num1 = 0, num2 = 0;
    int sum = 0;

    if (setjmp(env) == 0)
        printf("请输入两个整型值：");
    else
        printf("请重新输入两个整型数(注意:除数不能为0):");

    scanf("%d-%d", &num1, &num2);

    sum = div(num1, num2);

    printf("%d / %d = %d\n", num1, num2, sum);

    return 0;
}
