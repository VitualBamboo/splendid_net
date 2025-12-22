# src/drivers/dpdk/CMakeLists.txt

# 1. 定义静态库
add_library(driver_dpdk STATIC dpdk_device.c)

# 2. 链接 DPDK::SDK (注意必须是 PUBLIC，原因同 pcap)
# 这样 driver_dpdk 的头文件里包含 <rte_common.h> 时，使用者也能找到路径
target_link_libraries(driver_dpdk PUBLIC DPDK::SDK)

# 3. 公开自己的头文件路径
target_include_directories(driver_dpdk PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})