# splendid_net

轻量级手写 TCP/IP 协议栈 (xnet_tiny)

## 简介

xnet_tiny 是一个从零编写的轻量级 TCP/IP 协议栈，采用纯 C 语言实现，支持跨平台运行。项目参考了 lwIP 的设计思想，但做了大幅简化，适合学习和嵌入式场景。

## 功能特性

### 协议支持
| 层级 | 协议 |
|------|------|
| 链路层 | Ethernet, ARP |
| 网络层 | IP, ICMP |
| 传输层 | TCP, UDP |
| 应用层 | DHCP, HTTP, Time |

### 核心功能
- 完整的 TCP 状态机 (CLOSED → LISTEN → SYN_RECVD → ESTABLISHED → FIN_WAIT → CLOSED)
- TCP 重传与拥塞控制基础支持
- UDP 收发
- ARP 缓存与超时机制
- DHCP 动态 IP 获取
- 静态 IP 配置

### 平台支持
- **Windows**: Npcap 驱动
- **Linux (Ubuntu)**: DPDK 高性能驱动
- **Linux (Debian)**: TAP/TUN 虚拟网卡驱动

## 软件架构

```
splendid_net
├── src/
│   ├── xnet_tiny/          # 核心协议栈
│   │   ├── arch/           # 平台适配层 (Win32/Linux)
│   │   ├── core/           # 核心管理 (内存、初始化)
│   │   ├── link/           # 链路层 (Ethernet, ARP)
│   │   ├── network/        # 网络层 (IP, ICMP)
│   │   ├── transport/      # 传输层 (TCP, UDP)
│   │   ├── netif/          # 网卡抽象层
│   │   └── app/            # 应用层 (DHCP)
│   ├── xnet_socket/        # BSD Socket 抽象层 (支持 JNA 调用)
│   ├── xnet_app/           # 示例应用 (HTTP服务器、时间服务器)
│   └── drivers/            # 底层驱动
│       ├── pcap/           # Windows Npcap
│       ├── dpdk/           # Linux DPDK
│       └── tap/            # Linux TAP
└── CMakeLists.txt          # 跨平台构建配置
```

## 快速开始

### 环境要求
- CMake 3.16+
- C99 兼容编译器
- 平台对应驱动:
  - Windows: [Npcap SDK](https://npcap.org/sdk/)
  - Linux: DPDK 或 TUN/TAP 支持

### 编译

**Windows (Npcap)**
```bash
cmake -B build -DXNET_ENV=windows
cmake --build build
```

**Linux (DPDK)**
```bash
cmake -B build -DXNET_ENV=ubuntu
cmake --build build
```

**Linux (TAP)**
```bash
cmake -B build -DXNET_ENV=debian
cmake --build build
```

### 运行

```bash
./build/splendid_net
```

协议栈启动后会输出本机 IP 和 MAC 地址:
```
xnet stack initialized.
http server listening on port 80...
datetime server listening on port 13...
------------xnet running at 192.168.x.x, MAC: XX:XX:XX:XX:XX:XX
```

### 配置

在 `src/xnet_tiny/core/xnet_config.h` 中配置:
- `XNET_CFG_USE_STATIC_IP`: 使用静态 IP (1) 或 DHCP (0)
- `CFG_IP_ADDR`, `CFG_IP_MASK`, `CFG_IP_GW`: 静态 IP 配置

## 应用示例

### HTTP 服务器
访问 `http://<本机IP>/` 可查看简单的 HTTP 响应。

### 时间服务器
连接 `telnet <本机IP> 13` 可获取当前时间。

## 项目结构说明

- `xnet_def.h`: 公共定义 (类型、宏、结构体)
- `xnet_tiny.h`: 协议栈核心接口
- `xnet_packet_t`: 数据包缓冲区 (收发共用)
- `xtcp_pcb_t`: TCP 控制块 (连接状态管理)

## 许可证

MIT License
