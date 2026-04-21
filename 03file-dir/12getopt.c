/*使用getopt(3)解析命令行选项*/
#include <stdio.h>
#include <unistd.h>

int main(int argc, char *argv[])
{
    const char *optstring = "lhim";
    int ret = 0;

    while (1)
    {
        ret = getopt(argc, argv, optstring);
        if (ret == -1)
            break;
        switch (ret)
        {
        case 'l':
            printf("breakfast\n");
            break;
        case 'h':
            printf("lunch\n");
            break;
        case 'i':
            printf("dinner\n");
            break;
        case 'm':
            printf("supper\n");
            break;
        default:
            printf(" I Dont't Know!\n");
            break;
        }
    }
    return 0;
}