#include "xserver_http.h"
#include <stdio.h>
#include "xnet_tcp.h"

// FIFO 队列长度，与 PCB 数量一致
#define XHTTP_FIFO_SIZE XTCP_PCB_MAX_NUM

typedef struct _xhttp_fifo_t {
    xtcp_pcb_t* clients[XHTTP_FIFO_SIZE];
    uint8_t read_idx, write_idx;
} xhttp_fifo_t;

static xhttp_fifo_t http_fifo;

static void xhttp_fifo_init(xhttp_fifo_t* fifo) {
    fifo->write_idx = 0; //写指针，队尾
    fifo->read_idx = 0;  //读指针，队头
}

static xnet_status_t xhttp_fifo_enqueue(xhttp_fifo_t* fifo, xtcp_pcb_t* pcb) {
    // 1. 预判 tail 的下一步位置
    int next_write = (fifo->write_idx + 1) % XHTTP_FIFO_SIZE;

    // 2. 判满逻辑：如果下一步撞上了 read，说明只剩这一个保留空位了
    // 此时队列里实际存了 SIZE-1 个元素，刚好对应所有可用的39个 PCB
    if (next_write == fifo->read_idx) {
        return XNET_ERR_MEM;
    }

    fifo->clients[fifo->write_idx] = pcb;
    fifo->write_idx = next_write; // 更新 tail

    return XNET_OK;
}

static xtcp_pcb_t* xhttp_fifo_dequeue(xhttp_fifo_t* fifo) {
    // 1. 判空逻辑：头尾重合就是空
    if (fifo->read_idx == fifo->write_idx) {
        return NULL;
    }

    xtcp_pcb_t* client = fifo->clients[fifo->read_idx];
    fifo->read_idx = (fifo->read_idx + 1) % XHTTP_FIFO_SIZE;

    return client;
}

// [新增] 动态计算当前队列长度
static int xhttp_fifo_count(xhttp_fifo_t* fifo) {
    return (fifo->write_idx - fifo->read_idx + XHTTP_FIFO_SIZE) % XHTTP_FIFO_SIZE;
}

// 应用层回调方法
static xnet_status_t http_handler(xtcp_pcb_t* pcb, xtcp_event_t event) {

    switch (event) {
        case XTCP_EVENT_CONNECTED:
            printf("http: new client connected from %d.%d.%d.%d:%d\n",
               pcb->remote_ip.addr[0],
               pcb->remote_ip.addr[1],
               pcb->remote_ip.addr[2],
               pcb->remote_ip.addr[3],
               pcb->remote_port);
            xhttp_fifo_enqueue(&http_fifo, pcb);
            printf("fifo queue length: %d\n", xhttp_fifo_count(&http_fifo));
            break;

        case XTCP_EVENT_SENT:
            break;

        case XTCP_EVENT_DATA_RECEIVED:
            break;

        case XTCP_EVENT_CLOSED:
            printf("http: connection closed from %d.%d.%d.%d:%d\n",
                   pcb->remote_ip.addr[0],
                   pcb->remote_ip.addr[1],
                   pcb->remote_ip.addr[2],
                   pcb->remote_ip.addr[3],
                   pcb->remote_port);
            break;

        default:
            break;
    }
    return XNET_OK;
}

xnet_status_t xserver_http_create(uint16_t port) {
    xtcp_pcb_t* pcb = xtcp_pcb_new(http_handler);
    xtcp_pcb_bind(pcb, port);
    xtcp_pcb_listen(pcb);
    xhttp_fifo_init(&http_fifo);
    return XNET_OK;
}

void xserver_http_run(void) {

}