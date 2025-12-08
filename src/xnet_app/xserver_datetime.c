#include "xserver_datetime.h"
#include <time.h>
#include <string.h>

#include "xnet_udp.h"

#define TIME_STR_SIZE         128

xnet_status_t datetime_handler(xudp_socket_t * udp, xip_addr_t* src_ip, uint16_t src_port, xnet_packet_t* packet) {
    time_t rawtime;
    const struct tm * timeinfo;
    xnet_packet_t* tx_packet;
    size_t str_size;

    tx_packet = xnet_alloc_tx_packet(TIME_STR_SIZE);

    time(&rawtime);
    timeinfo = localtime(&rawtime);
    str_size = strftime((char*)tx_packet->data, TIME_STR_SIZE, "%A %B %d, %Y %T %z", timeinfo);

    // 注意：这里缺少 xudp_out/xip_out 的发送逻辑

    return XNET_OK;
}

xnet_status_t xserver_datetime_create(uint16_t port) {
    xudp_socket_t * udp = xudp_open(datetime_handler);
    xudp_bind(udp, port);
    return XNET_OK;
}