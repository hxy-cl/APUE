/*通过GID以及/etc/group文件获取该组的组名*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <grp.h>

int main(int argc, char *argv[])
{
    gid_t gid;
    struct group *grp;

    if (argc < 2)
    {
        fprintf(stderr, "用法: %s <GID>\n", argv[0]);
        return EXIT_FAILURE;
    }
    gid = atoi(argv[1]);
    grp = getgrgid(gid);
    if (grp == NULL)
    {
        fprintf(stderr, "未找到 GID %d\n", gid);
        return EXIT_FAILURE;
    }
    printf("%s\n", grp->gr_name);
    return EXIT_SUCCESS;
}