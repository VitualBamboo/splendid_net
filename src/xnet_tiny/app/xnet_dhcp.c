#include "xnet_dhcp.h"
#include "xnet_udp.h"
#include <stdio.h>
#include <string.h>

// 引入协议栈的本机 MAC 地址
extern uint8_t xnet_local_mac[XNET_MAC_ADDR_SIZE];

// 当前 DHCP 的状态
static xnet_dhcp_state_t dhcp_state = DHCP_STATE_DISABLED;
// 记录上一次发包的时间，用于超时重传
static xnet_time_t dhcp_timer;

// 我们用一个固定的事务 ID，方便在 Wireshark 里一眼认出它！
#define DHCP_MY_XID 0x11223344

// DHCP 专属的 UDP Socket
static xudp_pcb_t *dhcp_socket = NULL;

// 将来接收 DHCP Offer 时会触发的回调函数
// 🌟 升级版：接收并解析 DHCP 回包
static xnet_status_t dhcp_udp_handler(xudp_pcb_t *socket, xip_addr_t *src_ip, uint16_t src_port, xnet_packet_t *packet) {
    // 1. 长度防呆检查
    if (packet->len < sizeof(xnet_dhcp_hdr_t)) {
        return XNET_ERR_PARAM;
    }

    // 2. 将数据强转为 DHCP 头部结构体
    xnet_dhcp_hdr_t *hdr = (xnet_dhcp_hdr_t *)packet->data;

    // 3. 极其严苛的身份核对：
    // - 必须是服务器的回包 (OP == REPLY)
    // - 魔术字必须对得上
    // - 最关键的：事务 ID (XID) 必须是咱们自己发出去的那个！
    if (hdr->op != DHCP_OP_REPLY ||
        hdr->magic_cookie != swap_order32(DHCP_MAGIC_COOKIE) ||
        hdr->xid != swap_order32(DHCP_MY_XID)) {
        return XNET_OK; // 不是我的包，或者不是合法的 DHCP 包，直接扔掉
        }

    // 4. 解析咱们最关心的东西：服务器分配的 IP (yiaddr - Your IP)
    // 注意 yiaddr 是网络字节序的 uint32_t，我们把它强转成 bytes 数组方便打印
    uint8_t *offered_ip = (uint8_t *)&hdr->yiaddr;

    printf(">> [DHCP] BINGO! Received DHCP Reply!\n");
    printf(">> [DHCP] Server %d.%d.%d.%d offered us IP: %d.%d.%d.%d\n",
            src_ip->addr[0], src_ip->addr[1], src_ip->addr[2], src_ip->addr[3],
            offered_ip[0], offered_ip[1], offered_ip[2], offered_ip[3]);

    // TODO: 提取 Options 里的消息类型 (判断是 Offer 还是 Ack)
    // TODO: 提取服务器真实 IP (Option 54)
    // TODO: 状态机切换，发送 Request 包正式索要这个 IP

    return XNET_OK;
}

// 组装并发送 Discover 的核心函数
static void dhcp_send_discover(void) {
    // 1. 申请一个数据包 (大小 = DHCP头 + 一点点 Options 的空间)
    // 注意：这里调用的是你协议栈底层的发包分配函数
    uint16_t total_len = sizeof(xnet_dhcp_hdr_t) + 4; // 先算个大概，后面调整
    xnet_packet_t *packet = xnet_alloc_tx_packet(total_len);

    // 2. 将数据区强制转换为 DHCP 头部指针，并全部清零
    xnet_dhcp_hdr_t *hdr = (xnet_dhcp_hdr_t *)packet->data;
    memset(hdr, 0, sizeof(xnet_dhcp_hdr_t));

    // 3. 填充固定字段 (这就是 C 语言指针操作最爽的地方)
    hdr->op = DHCP_OP_REQUEST;
    hdr->htype = 1;         // 以太网
    hdr->hlen = 6;          // MAC 地址长度
    hdr->xid = swap_order32(DHCP_MY_XID); // 转换字节序 (如果有的话，没有直接赋值 0x11223344)
    hdr->flags = swap_order16(0x8000);    // 告诉服务器：“我没 IP，请务必用全网广播回我！”

    // 4. 填入你的真实 MAC 地址 (非常关键，服务器靠这个认人)
    memcpy(hdr->chaddr, xnet_local_mac, XNET_MAC_ADDR_SIZE);

    // 5. 注入灵魂：魔术字
    hdr->magic_cookie = swap_order32(DHCP_MAGIC_COOKIE);

    // 6. 徒手拼装变长的 Options (紧跟在 magic_cookie 之后)
    uint8_t *options = packet->data + sizeof(xnet_dhcp_hdr_t);
    int opt_idx = 0;

    // Option 53: DHCP 消息类型 (Message Type) = Discover (1)
    options[opt_idx++] = 53; // Type
    options[opt_idx++] = 1;  // Length
    options[opt_idx++] = 1;  // Value (1 = Discover)

    // Option 255: 结束符 (End) - 告诉服务器选项结束了
    options[opt_idx++] = 255;

    // 7. 修正包的最终真实长度
    packet->len = sizeof(xnet_dhcp_hdr_t) + opt_idx;

    printf(">> [DHCP] Sending Discover packet (XID: 0x%08X, len: %d bytes)...\n",
            DHCP_MY_XID, packet->len);

    xip_addr_t dest_ip = {{255, 255, 255, 255}};

    // 通过刚才绑定的 68 端口，将包发送到 67 端口
    if (dhcp_socket) {
        xudp_send_to(dhcp_socket, &dest_ip, 67, packet);
    }
}

void xnet_dhcp_init(void) {
    printf(">> [DHCP] Initializing DHCP Client...\n");
    memset(&xnet_local_ip, 0, sizeof(xip_addr_t));

    // 申请 Socket 并绑定在 68 端口
    if (dhcp_socket == NULL) {
        dhcp_socket = xudp_alloc_socket(dhcp_udp_handler);
        if (dhcp_socket) {
            xudp_bind_socket(dhcp_socket, 68);
        } else {
            printf(">> [DHCP] Panic: Failed to alloc UDP socket!\n");
        }
    }

    dhcp_state = DHCP_STATE_INIT;
    dhcp_timer = xsys_get_time();
}

void xnet_dhcp_poll(void) {
    // 如果禁用了 DHCP，直接退出
    if (dhcp_state == DHCP_STATE_DISABLED) return;

    // DHCP 的 DORA 状态机
    switch (dhcp_state) {
        case DHCP_STATE_INIT:
            printf(">> [DHCP] State: INIT. (Ready to send Discover)\n");
            dhcp_send_discover();

            // 发送完后，状态切换为等待回应
            dhcp_state = DHCP_STATE_REQUESTING;
            dhcp_timer = xsys_get_time(); // 记录发包时间
            break;

        case DHCP_STATE_REQUESTING:
            // TODO: 检查是否收到了 Offer 包

            // 简单的超时机制：如果 3 秒都没人理我，重新回到 INIT 状态发广播
            if (xnet_check_tmo(&dhcp_timer, 3)) {
                printf(">> [DHCP] Timeout! No Offer received. Retrying...\n");
                dhcp_state = DHCP_STATE_INIT;
            }
            break;

        case DHCP_STATE_BOUND:
            // 已经拿到 IP 了，这里以后可以处理“租期续约 (Renew)”的逻辑
            // 目前先什么都不做
            break;

        default:
            break;
    }
}