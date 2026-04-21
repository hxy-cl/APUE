/*通过GID以及/etc/group文件获取该组的组名*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define LINE_MAX 1024

int main(int argc, char *argv[])
{
    gid_t target_gid;
    char line[LINE_MAX];
    FILE *fp;
    int found = 0;
    char *groupname = NULL;
    unsigned long gid;
    char *token, *saveptr;

    // 1. 获取目标 GID
    if (argc < 2)
    {
        fprintf(stderr, "用法: %s <GID>\n", argv[0]);
        return EXIT_FAILURE;
    }
    errno = 0;
    target_gid = strtoul(argv[1], NULL, 10);
    if (errno != 0)
    {
        perror("无效的 GID");
        return EXIT_FAILURE;
    }

    // 2. 打开 /etc/group
    fp = fopen("/etc/group", "r");
    if (fp == NULL)
    {
        perror("打开 /etc/group 失败");
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
        groupname = token; // 第一个字段：组名

        token = strtok_r(NULL, ":", &saveptr); // 跳过密码字段
        if (!token)
            continue;

        token = strtok_r(NULL, ":", &saveptr); // 第三个字段：GID
        if (!token)
            continue;

        gid = strtoul(token, NULL, 10);
        if (gid == target_gid)
        {
            found = 1;
            break;
        }
    }

    fclose(fp);

    // 4. 输出结果
    if (found)
    {
        printf("%s\n", groupname);
        return EXIT_SUCCESS;
    }
    else
    {
        fprintf(stderr, "未找到 GID %d 对应的组\n", target_gid);
        return EXIT_FAILURE;
    }
}