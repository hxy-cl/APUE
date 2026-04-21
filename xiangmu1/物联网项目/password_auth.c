/* shadow密码认证模块 */
#include "iot_gateway.h"        // 项目公共头文件（可能包含日志等）
#include <shadow.h>             // 提供 getspnam 函数，读取 /etc/shadow
#include <crypt.h>              // 提供 crypt 函数，加密密码
#include <ctype.h>              // 提供字符判断函数：isdigit, isupper, islower

// 用户认证：验证用户名和密码是否匹配系统 shadow 文件中的记录
// 参数: username - 用户名（如 "root"）
//       password - 明文密码
// 返回值: 1 表示认证成功，0 表示失败（用户名不存在或密码错误）
int auth_user(const char *username, const char *password) {
    // getspnam: 根据用户名获取 shadow 文件中的密码条目（需要 root 权限）
    struct spwd *sp = getspnam(username);
    if (!sp) return 0;                      // 用户不存在，认证失败
    // crypt: 对明文密码使用 shadow 中存储的 salt 进行加密
    char *encrypted = crypt(password, sp->sp_pwdp);
    // 比较加密结果与 shadow 中的密文是否一致
    return strcmp(encrypted, sp->sp_pwdp) == 0;
}

// 检查密码强度：至少8位，包含数字、大写字母、小写字母
// 参数: pass - 待检查的密码字符串
// 返回值: 1 表示强度合格，0 表示不合格
int check_password_strength(const char *pass) {
    if (strlen(pass) < 8) return 0;         // 长度不足8位，直接失败
    int has_digit = 0, has_upper = 0, has_lower = 0; // 标记是否有数字、大写、小写
    for (const char *p = pass; *p; p++) {   // 遍历每个字符
        if (isdigit(*p)) has_digit = 1;     // 包含数字
        else if (isupper(*p)) has_upper = 1;// 包含大写字母
        else if (islower(*p)) has_lower = 1;// 包含小写字母
    }
    // 必须同时包含数字、大写、小写才算合格
    return has_digit && has_upper && has_lower;
}
