//
// Created by efairy520 on 2025/12/7.
//

#include "xnet_udp.h"
#include "xnet_ip.h"

#include <string.h>

static xudp_socket_t udp_socket_pool[XUDP_CFG_MAX_UDP];

void xudp_init(void) {
    memset(udp_socket_pool, 0, sizeof(udp_socket_pool));
}

void xudp_in(xudp_socket_t* udp_socket, xip_addr_t* src_ip, xnet_packet_t* packet)
{
    xudp_hdr_t* udp_hdr = (xudp_hdr_t*) packet->data;
    uint16_t pre_checksum;
    uint16_t src_port;

    // 1. 长度校验
    if ((packet->length < sizeof(xudp_hdr_t)) || (packet->length < swap_order16(udp_hdr->total_len))) {
        return; // 长度校验失败，丢弃数据包
    }

    // 2. 校验和
    pre_checksum = udp_hdr->checksum;
    udp_hdr->checksum = 0;

    // 3. UDP 校验和 可选，只有不为 0 时才需要验证
    if (pre_checksum != 0) {
        // 使用伪头部 (Pseudo-Header) 机制计算校验和，参数包括源IP、本地IP、协议类型、数据指针和长度
        uint16_t checksum = checksum_peso(src_ip, &xnet_local_ip, XNET_PROTOCOL_UDP,
                                          (uint16_t *)udp_hdr, swap_order16(udp_hdr->total_len));

        // 协议规定：如果计算结果为 0，则必须用 0xFFFF 表示（因为 0 代表未启用校验和）
        checksum = (checksum == 0) ? 0xFFFF : checksum;

        // 比较计算结果和原始校验和
        if (checksum != pre_checksum) {
            return; // 校验和验证失败，丢弃数据包
        }
    }

    // 4. 处理数据包
    // 将源端口号从网络字节序转换为主机字节序
    src_port = swap_order16(udp_hdr->src_port);

    // 移除 UDP 头部，数据包指针 (packet->data) 现在指向 UDP 有效载荷 (Payload)
    remove_header(packet, sizeof(xudp_hdr_t));

    // 检查 socket 是否有注册的处理函数 (handler)
    if (udp_socket->handler) {
        // 调用注册的处理函数，将数据包转发给上层应用
        udp_socket->handler(udp_socket, src_ip, src_port, packet);
    }
}

xudp_socket_t* xudp_open(xudp_handler_t handler) {
    // 1. 遍历资源池
    for (xudp_socket_t* cur = udp_socket_pool; cur < &udp_socket_pool[XUDP_CFG_MAX_UDP]; cur++) {

        // 2. 检查是否占用
        if (cur->state == XUDP_STATE_FREE) {
            cur->state = XUDP_STATE_USED;
            cur->local_port = 0; // 端口先置 0，表示还没绑定具体端口（稍后调用 bind）
            cur->handler = handler; // 挂载回调函数
            return cur;
        }
    }
    return NULL;
}

void xudp_close(xudp_socket_t* udp_socket) {
    udp_socket->state = XUDP_STATE_FREE;
}

xudp_socket_t* xudp_find(uint16_t port) {
    for (xudp_socket_t* curr = udp_socket_pool; curr < &udp_socket_pool[XUDP_CFG_MAX_UDP]; curr++) {
        if (curr->state == XUDP_STATE_USED && curr->local_port == port) {
            return curr;
        }
    }
    return NULL;
}

xnet_status_t xudp_bind(xudp_socket_t* udp_socket, uint16_t port) {
    // 1. 是否已占用
    for (xudp_socket_t* curr = udp_socket_pool; curr < &udp_socket_pool[XUDP_CFG_MAX_UDP]; curr++) {
        if (curr->state == XUDP_STATE_USED && curr->local_port == port) {
            return XNET_ERR_BINDED;
        }
    }
    // 2. 绑定
    udp_socket->local_port = port;
    return XNET_OK;
}