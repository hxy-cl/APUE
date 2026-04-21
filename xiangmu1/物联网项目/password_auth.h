#ifndef __PASSWORD_AUTH_H    // 头文件保护宏：防止重复包含
#define __PASSWORD_AUTH_H

// 用户认证函数
// 参数: username - 用户名
//       password - 明文密码
// 返回值: 1 认证成功，0 失败
int auth_user(const char *username, const char *password);

// 检查密码强度函数
// 参数: pass - 待检查的密码字符串
// 返回值: 1 强度合格，0 不合格
int check_password_strength(const char *pass);

#endif   // __PASSWORD_AUTH_H 结束头文件保护
