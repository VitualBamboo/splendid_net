# src/xnet_port/CMakeLists.txt

# ==========================================
# 1. 准备源文件列表 (先不创建库)
# ==========================================
set(PORT_SRCS "")

if(WIN32)
    # Windows: 使用 Pcap 驱动
    list(APPEND PORT_SRCS
            port_pcap.c
            ${PROJECT_SOURCE_DIR}/src/drivers/pcap/pcap_device.c
    )
else()
    # Linux: 使用 DPDK
    list(APPEND PORT_SRCS port_dpdk.c)
endif()

# ==========================================
# 2. 【关键】必须先创建目标！
# ==========================================
add_library(xnet_port OBJECT ${PORT_SRCS})

# ==========================================
# 3. 统一设置通用头文件路径
# ==========================================
target_include_directories(xnet_port
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE
        ${PROJECT_SOURCE_DIR}/src/xnet_tiny
)

# ==========================================
# 4. 根据平台补充特定的配置 (此时 xnet_port 已经存在了，可以设置了)
# ==========================================
if(WIN32)
    # 【修复】把 drivers/pcap 的路径加进来
    target_include_directories(xnet_port PRIVATE
            ${PROJECT_SOURCE_DIR}/src/drivers/pcap
    )

    # 链接 Npcap
    target_link_libraries(xnet_port PRIVATE Npcap::SDK)
else()
    # 链接 DPDK
    target_link_libraries(xnet_port PRIVATE DPDK::SDK)
endif()