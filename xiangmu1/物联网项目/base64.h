#ifndef __BASE64_H__                // 头文件保护宏：防止重复包含
#define __BASE64_H__                // 定义宏，表示头文件已被包含

/******************************
*名    称 : base64_encode()
*功    能 : base64的编码
*入口参数 : dest - 输出缓冲区，用于存储编码后的字符串（需保证足够空间）
*           src  - 输入二进制数据（或字符串）
*返回参数 : 0代表转换结束（成功）
*说    明 : 无
******************************/
int base64_encode(unsigned char *dest, const unsigned char *src);

/******************************
*名    称 : base64_decode()
*功    能 : base64的解码
*入口参数 : dest - 输出缓冲区，用于存储解码后的二进制数据
*           src  - 输入Base64编码的字符串
*返回参数 : 0代表转换结束（成功）
*说    明 : 无
******************************/
int base64_decode(unsigned char *dest, const char *src);

#endif                              // __BASE64_H__ 结束头文件保护
