#include "xsocket.h"
#include "xnet_tcp.h"
#include "xnet_tiny.h"

#include <stddef.h>

// 连接队列深度（listener backlog）
#define XSOCKET_BACKLOG 10

// xsocket_read 默认最大 poll 次数（避免无限阻塞）
#define XSOCKET_READ_DEFAULT_POLLS 2000

xsocket_t* xsocket_open(void) {
    // S 方案：accept 队列在 listener pcb 内部维护，因此不需要全局 ctx
    xtcp_pcb_t* pcb = xtcp_pcb_new(NULL);
    if (!pcb) return NULL;
    return (xsocket_t*)pcb;
}

void xsocket_close(xsocket_t* socket) {
    if (!socket) return;
    xtcp_pcb_close((xtcp_pcb_t*)socket);
}

xnet_status_t xsocket_bind(xsocket_t* socket, uint16_t port) {
    if (!socket) return XNET_ERR_PARAM;
    return xtcp_pcb_bind((xtcp_pcb_t*)socket, port);
}

xnet_status_t xsocket_listen(xsocket_t* socket) {
    if (!socket) return XNET_ERR_PARAM;

    xtcp_pcb_t* pcb = (xtcp_pcb_t*)socket;
    xnet_status_t r = xtcp_pcb_listen(pcb);
    if (r == XNET_OK) {
        // backlog 由 socket 层统一配置
        pcb->backlog = XSOCKET_BACKLOG;
    }
    return r;
}

xsocket_t* xsocket_accept(xsocket_t* socket) {
    if (!socket) return NULL;
    // 不要在这里 xnet_poll()；由 super loop 统一 poll
    return (xsocket_t*)xtcp_accept((xtcp_pcb_t*)socket);
}

// 写：仍保持你原来的“伪阻塞”模式（会 poll，但不会无限卡住太久）
// 注意：更理想是写也提供 try_write，然后上层状态机驱动。
// 但 Daytime 写很短，先这样够用。
int xsocket_write(xsocket_t* socket, const char* data, int len) {
    if (!socket || !data || len <= 0) return 0;

    xtcp_pcb_t* pcb = (xtcp_pcb_t*)socket;
    int sent_total = 0;

    while (len > 0) {
        int curr = xtcp_send(pcb, (uint8_t*)data, (uint16_t)len);
        if (curr < 0) return -1; // 连接断开或状态不对

        // xtcp_send 可能因为 tx_buf 满返回 0（你的实现里基本不会，但防一下）
        if (curr == 0) {
            // 让出一次 poll，等待 ACK 释放缓冲
            xnet_poll();
            continue;
        }

        len -= curr;
        data += curr;
        sent_total += curr;

        // 驱动发送（轻量）
        xnet_poll();
    }

    return sent_total;
}

// 连接是否还“可读/可等”的状态判断
static int xsocket_is_alive_for_read(const xtcp_pcb_t* pcb) {
    if (!pcb) return 0;

    switch (pcb->state) {
        case XTCP_STATE_ESTABLISHED:
        case XTCP_STATE_CLOSE_WAIT:   // 对端 FIN 后你仍可能有数据可读
        case XTCP_STATE_FIN_WAIT_1:
        case XTCP_STATE_FIN_WAIT_2:
            return 1;
        default:
            return 0;
    }
}

// 非阻塞读：一次尝试
int xsocket_try_read(xsocket_t* socket, char* buf, int max_len) {
    if (!socket || !buf || max_len <= 0) return 0;

    xtcp_pcb_t* pcb = (xtcp_pcb_t*)socket;

    // 先从 rx_buf 取（不会阻塞）
    int n = xtcp_recv(pcb, (uint8_t*)buf, (uint16_t)max_len);
    if (n > 0) return n;

    // 没数据：看连接是否还活着
    if (xsocket_is_alive_for_read(pcb)) {
        return 0;   // EWOULDBLOCK
    }
    return -1;      // 连接已关闭/异常
}

// 带超时读：最多 poll max_polls 次，仍无数据则返回 0
int xsocket_read_timeout(xsocket_t* socket, char* buf, int max_len, int max_polls) {
    if (!socket || !buf || max_len <= 0) return 0;
    if (max_polls <= 0) max_polls = 1;

    xtcp_pcb_t* pcb = (xtcp_pcb_t*)socket;

    for (int i = 0; i < max_polls; i++) {
        int n = xtcp_recv(pcb, (uint8_t*)buf, (uint16_t)max_len);
        if (n > 0) return n;

        // 连接死了
        if (!xsocket_is_alive_for_read(pcb)) {
            return -1;
        }

        // 关键：只 poll 一次，让协议栈继续前进，但不会无限霸占 CPU
        xnet_poll();
    }

    return 0; // 超时仍无数据
}

// 兼容旧接口：默认带上限超时
int xsocket_read(xsocket_t* socket, char* buf, int max_len) {
    return xsocket_read_timeout(socket, buf, max_len, XSOCKET_READ_DEFAULT_POLLS);
}
