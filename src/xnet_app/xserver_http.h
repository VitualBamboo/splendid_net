//
// Created by efairy520 on 2025/12/12.
//

#ifndef XSERVER_HTTP_H
#define XSERVER_HTTP_H

#include "xnet_def.h"

xnet_status_t xhttp_server_create(uint16_t port);
void xhttp_server_poll(void);

#endif //XSERVER_HTTP_H
