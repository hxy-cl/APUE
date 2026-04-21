#include "stdio.h"                  // 标准输入输出库（本文件中未直接使用，但可能依赖NULL等）
#include "base64.h"                 // 包含公开函数声明及常量

// 自定义字符串长度函数（模拟strlen）
int mystrlen(const char *);         // 函数声明
// 自定义字符查找函数（模拟strchr）
char *mystrchr(const char *, int);  // 函数声明
// 在字符串中查找字符并返回索引，未找到返回-1
int strchr_of_num(const char *, char);

// Base64编码表：所有64个合法字符，按索引0~63对应
const char *base64char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
const char end_char = '=';          // Base64填充字符

// 计算字符串长度（不包括结尾'\0'）
// 参数: s - 输入字符串
// 返回值: 字符个数
int mystrlen(const char *s)
{
    const char *sr = s;             // sr指向字符串起始位置
    while(*++s);                    // 先移动s再检查，直到遇到'\0'（此时s指向'\0'之后）
    return s - sr;                  // 指针差即为长度
}

// 在字符串中查找第一个匹配字符的位置
// 参数: s - 源字符串，c - 要查找的字符（转为int）
// 返回值: 找到则返回指向该字符的指针，否则返回NULL
char *mystrchr(const char *s, int c)
{
    while(*s != '\0' && *s != c)    // 未到末尾且当前字符不等于目标字符
        s++;                        // 继续向后移动
    return (char *)(*s == c ? s : NULL); // 若相等则返回该地址，否则返回NULL
}

// Base64编码函数
// 参数: dest - 输出缓冲区（需足够大，约为原数据长度的4/3倍加结束符）
//       src  - 输入二进制数据（可以包含任意字节，但本实现要求以'\0'结尾）
// 返回值: 成功返回0
int base64_encode(unsigned char *dest, const unsigned char *src)
{
    int i=0, j=0;                   // i为源数据索引（每次3字节），j为目标缓冲区索引
    unsigned char ind=0;            // 临时变量，存放6位索引值
    const int datalength = mystrlen((const char*)src); // 源数据长度（字节数）

    // 每次处理3个源字节，生成4个Base64字符
    for (i = 0; i < datalength; i += 3)
    {
        // ----- 处理第一个字符（取src[i]的高6位）-----
        ind = ((src[i] >> 2) & 0x3f);      // 右移2位，取低6位（0x3f=63）
        dest[j++] = base64char[(int)ind];  // 查表得到字符，存入目标并移动指针

        // ----- 处理第二个字符（结合src[i]的低2位和src[i+1]的高4位）-----
        ind = ((src[i] << 4) & 0x30);      // 左移4位后保留bit4~bit5（0x30=48）
        if ((i + 1) < datalength)          // 确保有第二个字节
        {
            ind |= ((src[i + 1] >> 4) & 0x0f); // 取src[i+1]的高4位，合并到ind
            dest[j++] = base64char[(int)ind];
        }
        else                               // 没有第二个字节（数据长度不是3的倍数）
        {
            dest[j++] = base64char[(int)ind]; // 输出当前ind对应的字符
            dest[j++] = end_char;            // 填充一个'='
            dest[j++] = end_char;            // 再填充一个'='
            break;                           // 编码结束，退出循环
        }

        // ----- 处理第三个字符（结合src[i+1]的低4位和src[i+2]的高2位）-----
        ind = ((src[i + 1] << 2) & 0x3c);    // 左移2位，保留bit2~bit5（0x3c=60）
        if (i + 2 < datalength)              // 确保有第三个字节
        {
            ind |= ((src[i + 2] >> 6) & 0x03); // 取src[i+2]的高2位，合并到ind
            dest[j++] = base64char[(int)ind];
            // ----- 处理第四个字符（取src[i+2]的低6位）-----
            ind = src[i + 2] & 0x3f;          // 直接取低6位
            dest[j++] = base64char[(int)ind];
        }
        else                               // 没有第三个字节（数据长度模3余1的情况）
        {
            dest[j++] = base64char[(int)ind]; // 输出当前ind对应的字符
            dest[j++] = end_char;            // 填充一个'='
            break;                           // 编码结束
        }
    }
    dest[j] = '\0';                     // 添加字符串结束符
    return 0;
}

// 在Base64编码表中查找字符，返回其索引
// 参数: str - 编码表字符串 base64char
//       c   - 待查找的字符
// 返回值: 如果找到则返回索引（0~63），否则返回-1
int strchr_of_num(const char *str, char c)
{
    const char *ind = mystrchr(str, c);     // 查找字符c在str中首次出现的位置
    return (int)(ind != NULL) ? ind - str : -1; // 计算索引，未找到则-1
}

// Base64解码函数
// 参数: dest - 输出缓冲区（存储解码后的二进制数据，调用者需保证足够空间）
//       src  - 输入Base64字符串（可能包含填充字符'='）
// 返回值: 成功返回0
int base64_decode(unsigned char *dest, const char *src)
{
    int i = 0, j=0;                 // i为源字符串索引（每次4字符），j为目标缓冲区索引
    int trans[4] = {0};             // 存放每4个Base64字符对应的6位索引值（0~63）
    for(i = 0; src[i] != '\0'; i += 4)  // 每次处理4个Base64字符
    {
        // 获取前两个字符在编码表中的索引
        trans[0] = strchr_of_num(base64char, src[i]);       // 第一个字符的索引
        trans[1] = strchr_of_num(base64char, src[i+1]);     // 第二个字符的索引
        // 重建第一个输出字节（8位）：由trans[0]的高6位 + trans[1]的高2位组成
        dest[j++] = ((trans[0] << 2) & 0xfc) | ((trans[1]>>4) & 0x03);
        // 0xfc = 11111100，提取trans[0]左移2位后的高6位；与trans[1]右移4位的低2位合并

        if(src[i + 2] == '=')       // 如果第三个字符是填充符，则解码结束（数据只产生1字节）
            continue;               // 跳过后续处理，直接进入下一轮循环
        else
            trans[2] = strchr_of_num(base64char, src[i + 2]); // 获取第三个字符索引
        // 重建第二个输出字节：由trans[1]的低4位 + trans[2]的高4位组成
        dest[j++] = ((trans[1] << 4) & 0xf0) | ((trans[2] >> 2) & 0x0f);
        // 0xf0 = 11110000，提取trans[1]左移4位后的高4位；与trans[2]右移2位的低4位合并

        if(src[i + 3] == '=')       // 如果第四个字符是填充符，则解码结束（数据只产生2字节）
            continue;               // 跳过后续处理
        else
            trans[3] = strchr_of_num(base64char, src[i + 3]); // 获取第四个字符索引
        // 重建第三个输出字节：由trans[2]的低2位 + trans[3]的低6位组成
        dest[j++] = ((trans[2] << 6) & 0xc0) | (trans[3] & 0x3f);
        // 0xc0 = 11000000，提取trans[2]左移6位后的高2位；与trans[3]的低6位合并
    }
    dest[j] = '\0';                 // 添加字符串结束符（注意：解码后的数据可能是二进制，但调用者通常按字符串使用）
    return 0;
}
