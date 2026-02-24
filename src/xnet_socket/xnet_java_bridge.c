// 文件位置: src/xnet_socket/xnet_java_bridge.c
#include <stdio.h>
#include "xnet_tiny.h"

// =======================================================
// 🌟 跨平台核心魔法：让编译器看系统下菜碟
// =======================================================
#if defined(_WIN32) || defined(_WIN64)
    // --- Windows 专属头文件与宏定义 ---
    #include <windows.h>

    #define XNET_EXPORT __declspec(dllexport)     // 导出给 JNA 用的修饰符
    #define XNET_THREAD_RETURN DWORD WINAPI       // 线程函数返回值
    #define XNET_THREAD_ARG LPVOID                // 线程函数参数
    #define XNET_SLEEP_MS(ms) Sleep(ms)           // 毫秒级休眠
    #define XNET_GET_TID() GetCurrentThreadId()   // 获取线程ID
#else
    // --- Linux/Unix 专属头文件与宏定义 ---
    #include <pthread.h>
    #include <unistd.h>
    #include <sys/syscall.h>

    #define XNET_EXPORT __attribute__((visibility("default"))) // Linux 导出修饰符
    #define XNET_THREAD_RETURN void*
    #define XNET_THREAD_ARG void*
    #define XNET_SLEEP_MS(ms) usleep((ms) * 1000) // Linux usleep 是微秒，乘1000变毫秒
    #define XNET_GET_TID() syscall(SYS_gettid)
#endif

// 这是一个标志位，控制线程运行
static volatile int is_running = 0;

// === 后台线程：协议栈的心脏 ===
XNET_THREAD_RETURN xnet_poll_thread(XNET_THREAD_ARG lpParam) {
    printf("[C-Bridge] Network Stack Thread Started (TID=%ld).\n", (long)XNET_GET_TID());

    while (is_running) {
        // 核心驱动函数
        xnet_poll();

        // 稍微休眠一点点，避免 CPU 占用率 100% 导致电脑卡顿
        XNET_SLEEP_MS(1);
    }
    return 0;
}

// === 暴露给 Java 的启动函数 ===
XNET_EXPORT void xnet_startup_global(void) {
    if (is_running) {
        printf("[C-Bridge] Stack is already running!\n");
        return;
    }

    // 1. 禁用缓冲，确保 Java 控制台能立刻看到日志
    setvbuf(stdout, NULL, _IONBF, 0);
    printf("[C-Bridge] Initializing xnet stack...\n");

    // 2. 初始化核心协议栈
    xnet_init();

    // 3. 启动后台线程
    is_running = 1;

#if defined(_WIN32) || defined(_WIN64)
    // Windows 平台：使用 CreateThread
    HANDLE hThread = CreateThread(
        NULL, 0, xnet_poll_thread, NULL, 0, NULL
    );

    if (hThread == NULL) {
        printf("[C-Bridge] Error: Failed to create Windows background thread!\n");
    } else {
        printf("[C-Bridge] Windows background thread created successfully.\n");
        CloseHandle(hThread); // 句柄用完关闭，线程会继续跑
    }
#else
    // Linux 平台：使用 POSIX pthread
    pthread_t thread_id;
    int ret = pthread_create(&thread_id, NULL, xnet_poll_thread, NULL);

    if (ret != 0) {
        printf("[C-Bridge] Error: Failed to create Linux background thread! (code: %d)\n", ret);
    } else {
        printf("[C-Bridge] Linux background thread created successfully.\n");
        // 分离线程，让它在后台独立运行，结束时自动释放系统资源
        pthread_detach(thread_id);
    }
#endif
}