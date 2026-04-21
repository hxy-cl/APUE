/* 配置加载模块 */
#include "iot_gateway.h"   // 包含项目公共头文件（宏定义、日志函数、传感器ID全局声明等）

// 全局传感器ID数组（定义，供其他模块使用）
char g_sht11_humi_id[32] = "16816";   // SHT11湿度传感器ID
char g_sht11_temp_id[32] = "16817";   // SHT11温度传感器ID
char g_adc_sensor_id[32] = "16828";   // ADC光敏传感器ID

// 内部配置结构体（静态，不对外暴露）
typedef struct {
    int tcp_port;               // TCP服务器端口
    int udp_port;               // UDP服务器端口
    int tbf_cps;                // 令牌桶速率（每秒令牌数）
    int tbf_burst;              // 令牌桶突发容量
    char auth_token[64];        // 认证令牌（字符串）
    char cloud_api_url[256];    // 云API的URL
    char cloud_api_token[256];  // 云API的Token
    int gateway_id;             // 网关ID
    int device_id;              // 设备ID
} config_t;

static config_t g_config;       // 静态配置实例（内部使用）

// 全局变量定义（供其他模块使用，如cloud_api.c）
char g_cloud_api_url[256];      // 云API URL（全局）
char g_cloud_api_token[256];    // 云API Token（全局）
int g_gateway_id;               // 网关ID（全局）
int g_device_id;                // 设备ID（全局）

/* 加载配置文件（路径由参数path指定） */
void config_load(const char *path) {
    FILE *fp = fopen(path, "r");     // 以只读方式打开配置文件
    if (!fp) {                        // 打开失败（文件不存在或无法读取）
        log_write(3, "Cannot open config, use defaults"); // 记录错误日志
        // 使用默认配置值（这些宏可能定义在iot_gateway.h中）
        g_config.tcp_port = TCP_PORT;
        g_config.udp_port = UDP_PORT;
        g_config.tbf_cps = TBF_CPS;
        g_config.tbf_burst = TBF_BURST;
        strcpy(g_config.auth_token, AUTH_TOKEN);
        strcpy(g_config.cloud_api_url, "http://iot.embsky.com/iot/device");
        strcpy(g_config.cloud_api_token, "eyJpYXQiOjE3NzMyODQzNzYsImV4cCI6MjA4ODY0NDM3NiwiYWxnIjoiSFMyNTYifQ.eyJpZCI6NTc3Nn0.Mz19_IBKMKlcGU4j0o-SabsyzlhzmPoG30PW5Zpky6s:");
        g_config.gateway_id = 5941;
        g_config.device_id = 5941;
    } else {                         // 文件打开成功，逐行解析
        char line[256];              // 每行缓冲区
        while (fgets(line, sizeof(line), fp)) {  // 读取一行
            if (line[0] == '#') continue;        // 跳过注释行（以#开头）
            char key[64], val[256];               // 键和值
            // 解析 "key=value" 格式，注意sscanf遇到空格会停止，所以值不能有空格
            if (sscanf(line, "%[^=]=%s", key, val) == 2) {
                if (strcmp(key, "tcp_port") == 0) g_config.tcp_port = atoi(val);
                else if (strcmp(key, "udp_port") == 0) g_config.udp_port = atoi(val);
                else if (strcmp(key, "tbf_cps") == 0) g_config.tbf_cps = atoi(val);
                else if (strcmp(key, "tbf_burst") == 0) g_config.tbf_burst = atoi(val);
                else if (strcmp(key, "auth_token") == 0) strcpy(g_config.auth_token, val);
                else if (strcmp(key, "cloud_api_url") == 0) strcpy(g_config.cloud_api_url, val);
                else if (strcmp(key, "cloud_api_token") == 0) strcpy(g_config.cloud_api_token, val);
                else if (strcmp(key, "gateway_id") == 0) g_config.gateway_id = atoi(val);
                else if (strcmp(key, "device_id") == 0) g_config.device_id = atoi(val);
                // 以下三个用于覆盖传感器ID（可选配置项）
                else if (strcmp(key, "sht11_humi_id") == 0) 
                    strcpy(g_sht11_humi_id, val);
                else if (strcmp(key, "sht11_temp_id") == 0) 
                    strcpy(g_sht11_temp_id, val);
                else if (strcmp(key, "adc_sensor_id") == 0) 
                    strcpy(g_adc_sensor_id, val);
            }
        }
        fclose(fp);   // 关闭文件
    }
    // 将内部配置同步到全局变量（供其他模块直接使用）
    strcpy(g_cloud_api_url, g_config.cloud_api_url);
    strcpy(g_cloud_api_token, g_config.cloud_api_token);
    g_gateway_id = g_config.gateway_id;
    g_device_id = g_config.device_id;
    
    // 记录加载完成的信息日志
    log_write(1, "Config loaded: tcp=%d udp=%d cloud_url=%s", 
              g_config.tcp_port, g_config.udp_port, g_config.cloud_api_url);
}
