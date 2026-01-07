#include "xserver_datetime.h"
#include "xsocket.h"      // 只依赖 Socket 接口
#include <time.h>
#include <stdio.h>
#include <string.h>

#include "xnet_tiny.h"

#define TIME_STR_SIZE 128

// 全局 Socket 句柄
static xsocket_t* datetime_server_socket;
static char time_buffer[TIME_STR_SIZE];

xnet_status_t xserver_datetime_create(uint16_t port) {
    // 1. 创建 Socket
    datetime_server_socket = xsocket_open();
    if (!datetime_server_socket) {
        return XNET_ERR_MEM;
    }

    // 2. 绑定端口 (通常是 13)
    xnet_status_t err = xsocket_bind(datetime_server_socket, port);
    if (err != XNET_OK) {
        xsocket_close(datetime_server_socket);
        datetime_server_socket = NULL;
        return err;
    }

    // 3. 开始监听
    err = xsocket_listen(datetime_server_socket);
    if (err != XNET_OK) {
        xsocket_close(datetime_server_socket);
        datetime_server_socket = NULL;
        return err;
    }

    return XNET_OK;
}

void xserver_datetime_poll(void) {
    if (!datetime_server_socket) return;

    // 非阻塞 accept
    xsocket_t* client = xsocket_accept(datetime_server_socket);
    if (!client) return;

    // 生成时间字符串
    time_t rawtime;
    struct tm* timeinfo;

    time(&rawtime);
    timeinfo = localtime(&rawtime);

    int len = 0;
    if (timeinfo) {
        len = (int)strftime(time_buffer, TIME_STR_SIZE, "%Y-%m-%d %H:%M:%S\r\n", timeinfo);
    }

    // strftime 失败或返回 0：兜底
    if (len <= 0) {
        const char* fallback = "1970-01-01 00:00:00\r\n";
        strncpy(time_buffer, fallback, TIME_STR_SIZE - 1);
        time_buffer[TIME_STR_SIZE - 1] = '\0';
        len = (int)strlen(time_buffer);
    }

    // 发送（Daytime 协议：发完就关）
    int w = xsocket_write(client, time_buffer, len);
    (void)w; // 目前不强依赖写成功与否，反正都要关闭

    // 立刻关闭：FIN 发出，后续状态机由主循环 xnet_poll 推进
    xsocket_close(client);
}
