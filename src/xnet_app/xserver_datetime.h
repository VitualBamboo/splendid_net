#ifndef XSERVER_DATTIME_H
#define XSERVER_DATTIME_H

#include "xnet_def.h"

xnet_status_t xserver_datetime_create(uint16_t port);
void xserver_datetime_poll(void);

#endif