# src/xnet_port/CMakeLists.txt

# 1. 准备源文件列表 (先不创建库)
set(PORT_SRCS "")

if(WIN32)
    # Windows: 使用 Pcap 驱动
    list(APPEND PORT_SRCS netif_pcap.c
    )
else()
    # Linux: 使用 DPDK
    list(APPEND PORT_SRCS netif_dpdk.c)
endif()

# 2. 必须先创建目标
add_library(xnet_netif STATIC ${PORT_SRCS})

# 3. 统一设置通用头文件路径
target_include_directories(xnet_netif
        PUBLIC
        .
        PRIVATE
        ../core
)


# 4. 根据平台补充特定的配置
if(WIN32)
    # 直接链接 driver_pcap 库
    target_link_libraries(xnet_netif PRIVATE driver_pcap)
else()
    # 链接 DPDK 驱动库
    target_link_libraries(xnet_netif PRIVATE driver_dpdk)
endif()