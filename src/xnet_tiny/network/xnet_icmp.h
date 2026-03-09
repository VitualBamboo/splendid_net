//
// Created by efairy520 on 2025/12/5.
//

#ifndef XNET_ICMP_H
#define XNET_ICMP_H

#include "xnet_ip.h"
#include "xnet_def.h"

#pragma pack(1)
typedef struct _xicmp_hdr_t {
    uint8_t type;       // 类型：8=Request, 0=Reply
    uint8_t code;       // 代码：0
    uint16_t checksum;  // 校验和
    uint16_t id;        // 标识符，一般是cmd窗口的进程id
    uint16_t seq;       // 序列号，1,2,3,4
    // 后面紧接着就是 Data，用指针访问即可
} xicmp_hdr_t;
#pragma pack()

#define XICMP_CODE_ECHO_REQUEST     8           // ICMP请求码值
#define XICMP_CODE_ECHO_REPLY       0           // ICMP响应码值

#define XICMP_TYPE_UNREACH          3
#define XICMP_CODE_PORT_UNREACH     3
#define XICMP_CODE_PRO_UNREACH      2

/**
 * 初始化 ICMP 协议
 */
void xicmp_init(void);

/**
 * 接收 ICMP 请求
 * @param src_ip 来源 ip
 * @param packet 数据包
 */
void xicmp_in(xip_addr_t *src_ip, xnet_packet_t *packet);

/**
 * 异常响应 ICMP 请求
 * @param code
 * @param ip_hdr
 * @return
 */
xnet_status_t xicmp_dest_unreach(uint8_t code, xip_hdr_t *ip_hdr) ;

#endif //XNET_ICMP_H
