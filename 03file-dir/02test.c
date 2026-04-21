/*通过UID以及/etc/passwd文件获取该用户的用户名*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <pwd.h>

int main(int argc, char *argv[])
{
    uid_t uid;
    struct passwd *pwd;

    if (argc < 2)
    {
        fprintf(stderr, "Usage:%s+uid!\n", argv[0]);
        return EXIT_FAILURE;
    }

    uid = atoi(argv[1]);
    pwd = getpwuid(uid);

    if (pwd == NULL)
    {
        fprintf(stderr, "未找到 UID %d\n", uid);
        return EXIT_FAILURE;
    }

    printf("%s\n", pwd->pw_name);
    return EXIT_SUCCESS;
}