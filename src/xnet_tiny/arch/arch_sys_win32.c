/**
* arch_sys_win32.c
 * Windows 平台下的系统架构抽象实现
 */
#include <windows.h>      // 引入纯正的 Windows API
#include "xnet_tiny.h"    // 引入契约头文件，为了实现 xsys_get_time

// 履行通用契约：获取系统时间（单位：秒）
xnet_time_t xsys_get_time(void) {
    // GetTickCount64() 返回自 Windows 系统启动以来的毫秒数
    // 除以 1000 转换为协议栈需要的“秒”
    return (xnet_time_t)(GetTickCount64() / 1000);
}

// 封装一个系统级初始化函数
void xsys_init(void) {
    // 1. 强制控制台使用 UTF-8，彻底解决乱码
    SetConsoleOutputCP(CP_UTF8);

    // 2. 如果你以后需要初始化 Winsock 环境 (WSAStartup)，也全放在这里
    // ...
}