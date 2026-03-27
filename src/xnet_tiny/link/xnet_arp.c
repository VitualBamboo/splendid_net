//
// Created by efairy520 on 2025/10/21.
//
#include "xnet_arp.h"

#include "string.h"
#include "xnet_ethernet.h"

#define XARP_ENTRY_FREE		                0               // 空闲
#define XARP_ENTRY_OK		                1               // 就绪
#define XARP_ENTRY_RESOLVING	            2               // ARP表项正在解析

#define XARP_DEBUG_MODE                     1               // 是否开启ARP调试模式

#if XARP_DEBUG_MODE
    // --- 调试模式 ---
    #define XARP_CFG_ENTRY_OK_TMO           10              // ARP表项存活 10秒
    #define XARP_CFG_ENTRY_RESOLVING_TMO    3               // 等待回复 3秒
#else
    // --- 生产模式 ---
    #define XARP_CFG_ENTRY_OK_TMO           300             // ARP表项存活 5分钟 (标准)
    #define XARP_CFG_ENTRY_RESOLVING_TMO    1               // 等待回复 1秒
#endif

// 以下参数通用
#define XARP_TIMER_PERIOD                   1               // 扫描周期 1秒
#define XARP_CFG_MAX_RETRIES                3               // 重试 3次
#define XARP_CFG_TABLE_SIZE                 10              // 表容量

// ARP表项
typedef struct _xarp_entry_t {
    xip_addr_t ipaddr;                                      // ip地址
    uint8_t macaddr[XNET_MAC_ADDR_SIZE];                    // mac地址
    uint8_t state;                                          // 状态位
    uint16_t ttl;                                           // 剩余时间
    uint8_t retry_cnt;                                      // 剩余重试次数
} xarp_entry_t;

static xarp_entry_t arp_table[XARP_CFG_TABLE_SIZE];         // ARP表
static xnet_time_t arp_last_time;                           // ARP定时器，记录上一次扫描的时间

/**
 * 内部查找函数：根据目标 IP 地址在 ARP 表中寻找对应的表项
 * @param ipaddr 查找的ip地址
 * @return 找到的表项指针，未找到则返回 NULL
 */
static xarp_entry_t* xarp_find_by_ip(const uint8_t *ipaddr) {
    for (int i = 0; i < XARP_CFG_TABLE_SIZE; i++) {
        if (arp_table[i].state != XARP_ENTRY_FREE && xip_addr_eq(ipaddr, arp_table[i].ipaddr.addr)) {
            return &arp_table[i];
        }
    }
    return NULL;
}

/**
 * 从 ARP 表中寻找一个可用的表项
 * 策略：优先分配空闲项；若表满，则淘汰存活时间最短（最老）的表项（LRU）
 * @return 可用表项的指针
 */
static xarp_entry_t* xarp_get_free_entry(void) {
    // 默认拿第一个当“最老候选人”
    xarp_entry_t *oldest = &arp_table[0];

    for (int i = 0; i < XARP_CFG_TABLE_SIZE; i++) {
        // 只要遇到空座，立刻入座，毫不犹豫
        if (arp_table[i].state == XARP_ENTRY_FREE) {
            return &arp_table[i];
        }
        // 能走到这，说明当前项不是空闲的。顺手更新一下寿命最少的倒霉蛋
        if (arp_table[i].ttl < oldest->ttl) {
            oldest = &arp_table[i];
        }
    }

    // 如果循环结束还没 return，说明没空座了，直接把最老的交出去覆盖掉
    return oldest;
}

void xarp_init(void) {
    for (int i = 0; i < XARP_CFG_TABLE_SIZE; i++) {
        arp_table[i].state = XARP_ENTRY_FREE;
    }

    // 初始化ARP上一次扫描时间，为当前时间
    xnet_check_tmo(&arp_last_time, 0);
}

// 轮询ARP表项是否超时，超时则重新请求
void xarp_poll(void) {
    // 每隔 PERIOD（1秒） 执行一次
    if (xnet_check_tmo(&arp_last_time, XARP_TIMER_PERIOD)) {
        for (int i = 0; i < XARP_CFG_TABLE_SIZE; i++) {
            switch (arp_table[i].state) {
                // 对方IP没有响应，才会进到这里
                case XARP_ENTRY_RESOLVING:
                    // 每次进来，都过了PERIOD，所以--
                    if (arp_table[i].ttl <= XARP_TIMER_PERIOD) {     // PENDING超时，准备重试
                        if (arp_table[i].retry_cnt-- == 0) { // 重试次数用完，回收
                            arp_table[i].state = XARP_ENTRY_FREE;
                        } else {    // 重试次数没有用完，开始重试
                            xarp_make_request(&arp_table[i].ipaddr);
                            arp_table[i].state = XARP_ENTRY_RESOLVING;
                            arp_table[i].ttl = XARP_CFG_ENTRY_RESOLVING_TMO;
                        }
                    } else {
                        arp_table[i].ttl -= XARP_TIMER_PERIOD;
                    }
                    break;
                case XARP_ENTRY_OK:
                    // 每次进来，都过了PERIOD，所以--
                    if (arp_table[i].ttl <= XARP_TIMER_PERIOD) {     // OK超时，重新请求
                        xarp_make_request(&arp_table[i].ipaddr); // 想要测试，需要把虚拟机网络关闭，否则一直ok
                        arp_table[i].state = XARP_ENTRY_RESOLVING;
                        arp_table[i].ttl = XARP_CFG_ENTRY_RESOLVING_TMO;
                        arp_table[i].retry_cnt = XARP_CFG_MAX_RETRIES;
                    } else {
                        arp_table[i].ttl -= XARP_TIMER_PERIOD;
                    }
                    break;
                case XARP_ENTRY_FREE:
                    // ARP协议初始化后，默认是FREE状态
                    break;

            }
        }
    }
}

/**
 * 更新ARP表项
 * @param src_ip 源IP地址
 * @param mac_addr 对应的mac地址
 */
void update_arp_entry(uint8_t *src_ip, uint8_t *mac_addr) {
    xarp_entry_t *entry = xarp_find_by_ip(src_ip);

    if (entry == NULL) {
        entry = xarp_get_free_entry();
    }

    memcpy(entry->ipaddr.addr, src_ip, XNET_IPV4_ADDR_SIZE);
    memcpy(entry->macaddr, mac_addr, XNET_MAC_ADDR_SIZE);
    entry->state = XARP_ENTRY_OK;
    entry->ttl = XARP_CFG_ENTRY_OK_TMO;
    entry->retry_cnt = XARP_CFG_MAX_RETRIES;
}

/**
 * 解析指定的IP地址，如果不在ARP表项中，则发送ARP请求
 * @param ipaddr 查找的ip地址
 * @param mac_addr 返回的mac地址存储区
 * @return XNET_ERR_OK 查找成功，XNET_ERR_NONE 查找失败
 */
xnet_status_t xarp_resolve(const xip_addr_t *ipaddr, uint8_t **mac_addr) {
    xarp_entry_t *entry = xarp_find_by_ip(ipaddr->addr);

    if (entry != NULL) {
        // 匹配到了arp表项，直接返回 mac 地址
        if (entry->state == XARP_ENTRY_OK) {
            *mac_addr = entry->macaddr;
            return XNET_OK;
        }
        return XNET_ERR_NONE;
    }

    // 没有匹配到arp表项，发送arp请求
    entry = xarp_get_free_entry();
    memcpy(entry->ipaddr.addr, ipaddr->addr, XNET_IPV4_ADDR_SIZE);
    entry->state = XARP_ENTRY_RESOLVING;
    entry->ttl = XARP_CFG_ENTRY_RESOLVING_TMO;
    entry->retry_cnt = XARP_CFG_MAX_RETRIES;

    // 发送第一次请求
    xarp_make_request(ipaddr);

    return XNET_ERR_NONE;
}