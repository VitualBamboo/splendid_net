//
// Created by efairy520 on 2025/12/12.
//

#ifndef XSERVER_HTTP_H
#define XSERVER_HTTP_H

#include "xnet_def.h"

xnet_status_t xserver_http_create(uint16_t port);
void xserver_http_run(void);

#endif //XSERVER_HTTP_H
