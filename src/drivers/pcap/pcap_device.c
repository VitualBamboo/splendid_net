/**
 * 对 Npcap 底层 API 的封装
 */
#include <memory.h>
#include "pcap_device.h"

#if defined(WIN32)

#include <winsock.h>
#include <tchar.h>
#include <time.h>

#pragma comment(lib, "ws2_32.lib")  // 仅保留 win32 的网络库链接，pcap 的库链接已移至 CMake

static const char *read_num(const char *str, int *num) {
    const char *pstr = str;

    while ((*pstr < '0') || (*pstr > '9')) {
        if (*pstr == '\0') {
            return '\0';
        }

        pstr++;
    }

    *num = 0;
    while (*pstr) {
        char c = *pstr++;
        if ((c >= '0') && (c <= '9')) {
            *num = *num * 10 + c - '0';
        } else {
            break;
        }
    }

    return pstr;
}


/**
 * 调整npcap的搜索路径：该函数现在仅用于检查 Npcap 版本。
 * 路径调整工作已通过 CMake 自动部署 DLL 到应用目录的方式解决。
 */
static int load_pcap_lib() {
    static int dll_loaded = 0;
    int m_version, n_version;

    if (dll_loaded) {
        return 0;
    }

    // Npcap 的核心 DLL (wpcap.dll, Packet.dll) 应该已经被 CMake 复制到可执行文件目录
    // 因此我们不需要再设置 SetDllDirectory 或检查 C:\Windows\System32\Npcap 路径。

    // 检查版本号，要求必须比工程所用的高
    const char *v_str = pcap_lib_version();

    // 如果 pcap_lib_version() 返回空或解析失败，说明 DLL 加载有问题（但由于 CMake 复制，此情况应较少发生）
    if (!v_str) {
        goto error_end;
    }

    v_str = read_num(v_str, &m_version);
    read_num(v_str, &n_version);

    if ((m_version < NPCAP_VERSION_M) || ((m_version == NPCAP_VERSION_M) && (n_version < NPCAP_VERSION_N))) {
        _TCHAR title[256];

        wsprintf(title, _T("npcap版本号太老: %d.%d < %d.%d"), m_version, n_version, NPCAP_VERSION_M, NPCAP_VERSION_N);
        MessageBox(0,
            _T("1.请卸载所有已安装的npcap或者winpcap. \n2. 请安装最新版本npcap，或者安装课程提供的wireshark, 安装过程中安装其附带的npcap."),
            title,
            MB_ABORTRETRYIGNORE);
        return -1;
    }

    dll_loaded = 1;
    return 0;


error_end:
    MessageBox(0,
        _T("Npcap 驱动加载失败或版本信息无法读取。请确保已安装最新 Npcap，并重新启动程序。"),
        _T("npcap驱动加载失败"),
        MB_ABORTRETRYIGNORE);
    return -1;
}

#else   // Mac或者linux

#include <netinet/in.h>
#include <arpa/inet.h>

static int load_pcap_lib() {
    return 0;
}

#endif

// 以下所有函数 (pcap_find_device, pcap_show_list, pcap_device_open 等) 逻辑保持不变，
// 因为它们与 DLL 加载方式无关，只依赖于 pcap 库的 API。

/**
 * 找到指定IP地址的网卡名 (支持精确匹配和网段匹配)
 */
static int pcap_find_device(const char *ip, char *name_buf) {
    char err_buf[PCAP_ERRBUF_SIZE];
    pcap_if_t *pcap_if_list = NULL;
    struct in_addr dest_ip;

    inet_pton(AF_INET, ip, &dest_ip);

    if (pcap_findalldevs(&pcap_if_list, err_buf) < 0) {
        fprintf(stderr, ">> [PCAP Error] 获取系统网卡列表失败: %s\n", err_buf);
        return -1;
    }

    printf("\n========== 🔍 正在扫描系统物理/虚拟网卡 ==========\n");
    printf(">> 协议栈期望绑定的目标 IP 或网段: %s\n", ip);

    for (pcap_if_t *item = pcap_if_list; item != NULL; item = item->next) {
        if (item->addresses == NULL) continue;

        for (struct pcap_addr *pcap_addr = item->addresses; pcap_addr != NULL; pcap_addr = pcap_addr->next) {
            if (pcap_addr->addr->sa_family != AF_INET) continue;

            struct sockaddr_in *curr_addr = (struct sockaddr_in *)pcap_addr->addr;
            struct sockaddr_in *curr_mask = (struct sockaddr_in *)pcap_addr->netmask;

            char ip_str[INET_ADDRSTRLEN];
            inet_ntop(AF_INET, &curr_addr->sin_addr, ip_str, sizeof(ip_str));

            // 如果网卡没有掩码，默认使用 24 位掩码
            uint32_t mask = curr_mask ? curr_mask->sin_addr.s_addr : 0x00FFFFFF;

            printf("  - 发现网卡: [%s] IP: %s\n",
                   item->description ? item->description : item->name, ip_str);

            // 🌟 核心魔法：网段匹配！
            // 条件1: 完全相等 (匹配宿主机自身的IP)
            // 条件2: 网段相等 (IP & 掩码 == 目标IP & 掩码)
            if ((curr_addr->sin_addr.s_addr == dest_ip.s_addr) ||
                ((curr_addr->sin_addr.s_addr & mask) == (dest_ip.s_addr & mask))) {

                printf("  ✅ 成功匹配！决定使用该网卡进行底层抓包。\n");
                strcpy(name_buf, item->name);
                pcap_freealldevs(pcap_if_list);
                printf("==================================================\n\n");
                return 0;
            }
        }
    }

    printf("❌ 未找到任何与 %s 属于同一网段的网卡！\n", ip);
    printf("==================================================\n\n");
    pcap_freealldevs(pcap_if_list);
    return -1;
}

/**
 * 打开pcap设备接口
 * @param ip 打开网卡的指定ip
 * @param 给网卡设置mac
 */
pcap_t *pcap_device_open(const char *ip, const uint8_t *mac_addr, uint8_t poll_mode) {
    char err_buf[PCAP_ERRBUF_SIZE];
    struct bpf_program fp;
    bpf_u_int32 mask;
    bpf_u_int32 net;
    char filter_exp[256];
    char name_buf[256];
    pcap_t *pcap;

    if (load_pcap_lib() < 0) {
        fprintf(stderr, "pcap_open: load pcap dll failed! install it first\n");
        return (pcap_t*)0;
    }

    if (pcap_find_device(ip, name_buf) < 0) {
        fprintf(stderr, "pcap_open: no net card has ip: %s, use the following:\n", ip);
        return (pcap_t*)0;
    }

    if (pcap_lookupnet(name_buf, &net, &mask, err_buf) == -1) {
        printf("pcap_open: can't find use net card: %s\n", name_buf);
        net = 0;
        mask = 0;
    }

    // ================== 修改开始 ==================
    // 原始代码: pcap = pcap_open_live(name_buf, 65536, 1, 0, err_buf);
    // 问题原因: pcap_open_live 第4个参数为0表示无限等待缓冲填满，导致几十秒延迟

    // 新代码: 使用 pcap_create + pcap_activate 流程以支持“立即模式”
    pcap = pcap_create(name_buf, err_buf);
    if (pcap == NULL) {
        fprintf(stderr, "pcap_open: create pcap failed %s\n net card name: %s\n", err_buf, name_buf);
        return (pcap_t*)0;
    }

    // 1. 设置快照长度 (对应 open_live 的 65536)
    pcap_set_snaplen(pcap, 65536);

    // 2. 设置混杂模式 (对应 open_live 的 1)
    pcap_set_promisc(pcap, 1);

    // 3. 【核心修复】设置立即模式
    // 告诉内核收到包立刻拷贝给用户层，不要缓冲！
    pcap_set_immediate_mode(pcap, 1);

    // 4. 设置超时时间 (对应 open_live 的 0 -> 改为 1ms)
    // 即使在非阻塞模式下，设置一个极短的超时也比0(无限)安全
    pcap_set_timeout(pcap, 1);

    // 5. 激活句柄
    int status = pcap_activate(pcap);
    if (status < 0) {
        fprintf(stderr, "pcap_open: activate failed: %s\n", pcap_geterr(pcap));
        pcap_close(pcap);
        return (pcap_t*)0;
    } else if (status > 0) {
        // 警告信息，比如是在混杂模式下
        fprintf(stderr, "pcap_open: warning: %s\n", pcap_geterr(pcap));
    }
    // ================== 修改结束 ==================

    // 非阻塞模式读取，程序中使用查询的方式读
    if (pcap_setnonblock(pcap, 1, err_buf) != 0) {
        fprintf(stderr, "pcap_open: set none block failed: %s\n", pcap_geterr(pcap));
        pcap_close(pcap); // 失败记得关闭
        return (pcap_t*)0;
    }

    // 只捕获输入，不要捕获自己发出去的
    // 注：win平台似乎不支持这个选项
    if (pcap_setdirection(pcap, PCAP_D_IN) != 0) {
        // fprintf(stderr, "pcap_open: set direction not suppor: %s\n", pcap_geterr(pcap));
    }

    // 只捕获发往本接口与广播的数据帧。相当于只处理发往这张网卡的包
    sprintf(filter_exp,
            "(ether dst %02x:%02x:%02x:%02x:%02x:%02x or ether broadcast) and (not ether src %02x:%02x:%02x:%02x:%02x:%02x)",
            mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5],
            mac_addr[0], mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], mac_addr[5]);

    if (pcap_compile(pcap, &fp, filter_exp, 0, net) == -1) {
        printf("pcap_open: couldn't parse filter %s: %s\n", filter_exp, pcap_geterr(pcap));
        pcap_close(pcap);
        return (pcap_t*)0;
    }
    if (pcap_setfilter(pcap, &fp) == -1) {
        printf("pcap_open: couldn't install filter %s: %s\n", filter_exp, pcap_geterr(pcap));
        pcap_close(pcap);
        return (pcap_t*)0;
    }

    return pcap;
}

/**
 * 关闭Pcapif接口
 */
void pcap_device_close(pcap_t *pcap) {
    if (pcap == (pcap_t *)0) {
        fprintf(stderr, "pcap = 0");
        return;
    }
    pcap_close(pcap);
}

/**
 * 向网络接口发送数据包
 */
uint32_t pcap_device_send(pcap_t *pcap, const uint8_t *buffer, uint32_t length) {
    if (pcap_sendpacket(pcap, buffer, length) == -1) {
        fprintf(stderr, "pcap send: send packet failed!:%s\n", pcap_geterr(pcap));
        fprintf(stderr, "pcap send: pcaket size %d\n", length);
        return 0;
    }

    return 0;
}

/**
 * 从网络接口读取数据包
 */
uint32_t pcap_device_read(pcap_t *pcap, uint8_t *buffer, uint32_t length) {
    int err;
    struct pcap_pkthdr *pkthdr;
    const uint8_t *pkt_data;

    err = pcap_next_ex(pcap, &pkthdr, &pkt_data);
    if (err == 0) {
        return 0;
    } else if (err == 1) {     // 1 - 成功读取数据包, 0 - 没有数据包，其它值-出错
        memcpy(buffer, pkt_data, pkthdr->len);
        return pkthdr->len;
    }

    fprintf(stderr, "pcap_read: reading packet failed!:%s", pcap_geterr(pcap));
    return 0;
}
