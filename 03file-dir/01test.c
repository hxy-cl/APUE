/*通过UID以及/etc/passwd文件获取该用户的用户名*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define LINE_MAX 1024

int main(int argc, char *argv[])
{
    uid_t target_uid;
    char line[LINE_MAX];
    FILE *fp;
    int found = 0;
    char *username = NULL;
    unsigned long uid;
    char *token, *saveptr;

    // 1. 获取目标 UID
    if (argc < 2)
    {
        fprintf(stderr, "用法: %s <UID>\n", argv[0]);
        return EXIT_FAILURE;
    }
    errno = 0;
    target_uid = strtoul(argv[1], NULL, 10);
    if (errno != 0)
    {
        perror("无效的 UID");
        return EXIT_FAILURE;
    }

    // 2. 打开 /etc/passwd
    fp = fopen("/etc/passwd", "r");
    if (fp == NULL)
    {
        perror("打开 /etc/passwd 失败");
        return EXIT_FAILURE;
    }

    // 3. 逐行解析
    while (fgets(line, sizeof(line), fp))
    {
        // 去掉行尾换行符
        line[strcspn(line, "\n")] = '\0';

        // 使用 strtok_r 解析冒号分隔的字段
        token = strtok_r(line, ":", &saveptr);
        if (!token)
            continue;
        username = token; // 第一个字段：用户名

        token = strtok_r(NULL, ":", &saveptr); // 跳过密码字段
        if (!token)
            continue;

        token = strtok_r(NULL, ":", &saveptr); // 第三个字段：UID
        if (!token)
            continue;

        uid = strtoul(token, NULL, 10);
        if (uid == target_uid)
        {
            found = 1;
            break;
        }
    }

    fclose(fp);

    // 4. 输出结果
    if (found)
    {
        printf("%s\n", username);
        return EXIT_SUCCESS;
    }
    else
    {
        fprintf(stderr, "未找到 UID %d 对应的用户\n", target_uid);
        return EXIT_FAILURE;
    }
}