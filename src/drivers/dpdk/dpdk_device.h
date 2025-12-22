#ifndef DPDK_DEVICE_H
#define DPDK_DEVICE_H

// 1. 必须引入标准整数类型，否则 uint16_t 报错
#include <stdint.h>
#include <stdbool.h>

// 2. 引入 DPDK 核心头文件 (解决 rte_xxx 未定义问题)
#include <rte_eal.h>
#include <rte_ethdev.h>
#include <rte_mbuf.h>
#include <rte_cycles.h>
#include <rte_lcore.h>

// 3. 定义返回状态码 (解决 DPDK_OK/DPDK_ERR 未定义问题)
#define DPDK_OK  0
#define DPDK_ERR -1

// 4. 函数声明
int dpdk_device_init(void);
int dpdk_device_send(const void *data, uint16_t len);
int dpdk_device_read(void *buffer, uint16_t max_len);
void dpdk_device_get_mac(uint8_t *mac_buf);

#endif // DPDK_DEVICE_H