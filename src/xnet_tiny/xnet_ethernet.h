//
// Created by efairy520 on 2025/10/21.
//

#ifndef XNET_ETHERNET_H
#define XNET_ETHERNET_H

#include "xnet_tiny.h"

xnet_err_e ethernet_init(void);
void ethernet_poll(void);
xnet_err_e xarp_make_request(const xip_addr_u *target_ipaddr);
xnet_err_e ethernet_out_to(xnet_protocol_e protocol, const uint8_t *mac_addr, xnet_packet_t *packet);
xnet_err_e xarp_make_response(uint8_t *target_ip, uint8_t *target_mac);

#endif //XNET_ETHERNET_H
