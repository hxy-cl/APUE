/* 日志模块，按天滚动 */
#include "iot_gateway.h"        // 项目公共头文件（包含宏、日志级别定义等）
#include <stdarg.h>             // 提供可变参数处理（va_list, va_start, va_end）
#include <time.h>               // 提供时间函数（time, localtime, strftime）

static FILE *log_fp = NULL;     // 当前打开的日志文件指针，NULL表示未初始化或打开失败
static char log_dir[256];       // 日志文件所在目录路径（由log_init传入）
static char current_date[16];   // 当前日志文件对应的日期字符串（格式YYYYMMDD），用于检测是否需要滚动

// 检查是否需要滚动日志文件（按天切换）
// 参数: 无
// 返回值: 无
static void check_rollover(void) {
    time_t t = time(NULL);              // 获取当前系统时间（秒数）
    struct tm *tm = localtime(&t);      // 转换为本地时间结构体
    char date[16];                      // 临时存储今天的日期字符串
    // 格式化日期为 "YYYYMMDD"（例如 20260321）
    strftime(date, sizeof(date), "%Y%m%d", tm);
    // 如果今天的日期与当前日志文件的日期不同，则需要滚动
    if (strcmp(date, current_date) != 0) {
        if (log_fp) fclose(log_fp);     // 关闭旧的日志文件
        strcpy(current_date, date);     // 更新当前日期
        char path[512];                 // 完整的日志文件路径
        // 构造路径: 日志目录/iot_YYYYMMDD.log
        snprintf(path, sizeof(path), "%s/iot_%s.log", log_dir, date);
        log_fp = fopen(path, "a");      // 以追加模式打开新日志文件
        if (!log_fp) log_fp = stderr;   // 打开失败则回退到标准错误输出
    }
}

// 初始化日志系统
// 参数: logdir - 日志文件存储目录的路径（如 "./log"）
// 返回值: 无
void log_init(const char *logdir) {
    strcpy(log_dir, logdir);            // 保存日志目录路径
    mkdir(logdir, 0755);                // 创建目录（如果已存在则忽略错误）
    check_rollover();                   // 立即创建当天日志文件
}

// 写入日志（支持格式化输出和日志级别）
// 参数: level - 日志级别（0=DEBUG, 1=INFO, 2=WARN, 3=ERROR, 4=FATAL, 其他=UNK）
//       fmt   - 格式化字符串（类似printf）
//       ...   - 可变参数
// 返回值: 无
void log_write(int level, const char *fmt, ...) {
    if (!log_fp) return;                // 日志文件未初始化，直接返回

    check_rollover();                   // 先检查是否需要滚动（防止跨天时仍写旧文件）

    time_t t = time(NULL);              // 获取当前时间
    struct tm *tm = localtime(&t);      // 转换为本地时间
    // 打印时间戳 [HH:MM:SS]
    fprintf(log_fp, "[%02d:%02d:%02d] ", tm->tm_hour, tm->tm_min, tm->tm_sec);

    // 根据日志级别打印对应的标签（固定宽度5字符，便于对齐）
    switch (level) {
        case 0: fprintf(log_fp, "DEBUG "); break;
        case 1: fprintf(log_fp, "INFO  "); break;
        case 2: fprintf(log_fp, "WARN  "); break;
        case 3: fprintf(log_fp, "ERROR "); break;
        case 4: fprintf(log_fp, "FATAL "); break;
        default: fprintf(log_fp, "UNK   ");
    }

    // 处理可变参数并输出格式化内容
    va_list ap;                 // 定义可变参数列表变量
    va_start(ap, fmt);          // 初始化，指向第一个可变参数
    vfprintf(log_fp, fmt, ap);  // 将格式化输出写入日志文件
    va_end(ap);                 // 清理可变参数列表

    fprintf(log_fp, "\n");      // 每条日志末尾换行
    fflush(log_fp);             // 立即刷新缓冲区，确保日志及时写入磁盘
}
