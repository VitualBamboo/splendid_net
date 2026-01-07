#ifndef XSOCKET_H
#define XSOCKET_H

#include <stdint.h>
#include "xnet_def.h"

// socket 句柄（你当前用 xtcp_pcb_t* 强转即可）
typedef void xsocket_t;

// 创建/关闭
xsocket_t* xsocket_open(void);
void xsocket_close(xsocket_t* socket);

// 绑定/监听/accept
xnet_status_t xsocket_bind(xsocket_t* socket, uint16_t port);
xnet_status_t xsocket_listen(xsocket_t* socket);
xsocket_t* xsocket_accept(xsocket_t* socket);

// 写
int xsocket_write(xsocket_t* socket, const char* data, int len);

// 读：推荐用 try 版本（非阻塞）
// 返回：>0 读到字节数；0 暂无数据；-1 连接断开/错误
int xsocket_try_read(xsocket_t* socket, char* buf, int max_len);

// 读：带超时（用 poll 次数作为超时单位）
// 返回：>0 读到字节数；0 超时仍无数据；-1 连接断开/错误
int xsocket_read_timeout(xsocket_t* socket, char* buf, int max_len, int max_polls);

// 读：兼容旧接口（内部会超时，不会无限卡死）
// 返回：>0 读到字节数；0 超时；-1 连接断开/错误
int xsocket_read(xsocket_t* socket, char* buf, int max_len);

#endif // XSOCKET_H
