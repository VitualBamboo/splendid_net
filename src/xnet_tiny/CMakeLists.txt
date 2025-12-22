# src/xnet_tiny/CMakeLists.txt

# 1. 定义源文件列表 (带上新的目录前缀)
set(TINY_SRCS
        # 核心层
        core/xnet_tiny.c

        # 链路层
        link/xnet_ethernet.c

        # 网络层
        network/xnet_arp.c
        network/xnet_ip.c
        network/xnet_icmp.c

        # 传输层
        transport/xnet_udp.c
        transport/xnet_tcp.c
)

# 2. 创建静态库 (推荐做成静态库)
add_library(xnet_tiny STATIC ${TINY_SRCS})

# 3. 设置头文件搜索路径
# 这一步非常关键！它把所有子目录都加到了搜索路径里。
target_include_directories(xnet_tiny
        PUBLIC
        # 对外公开 core，因为外部需要引用 xnet_tiny.h 和 xnet_def.h
        ${CMAKE_CURRENT_SOURCE_DIR}/core

        PRIVATE
        # 内部模块之间互相引用，对外隐藏
        ${CMAKE_CURRENT_SOURCE_DIR}/link
        ${CMAKE_CURRENT_SOURCE_DIR}/network
        ${CMAKE_CURRENT_SOURCE_DIR}/transport
        # 【新增】承认依赖：告诉 Tiny 去哪里找适配器接口
        ${PROJECT_SOURCE_DIR}/src/xnet_port
)