# src/xnet_socket/CMakeLists.txt

set(SOCKET_SRCS
        xsocket.c
        xnet_java_bridge.c   # <--- 新增这一行
)

add_library(xnet_socket SHARED ${SOCKET_SRCS})

# 设置头文件路径
target_include_directories(xnet_socket
        PUBLIC
        # 1. 对外公开 xsocket.h 所在的目录
        ${CMAKE_CURRENT_SOURCE_DIR}

        PRIVATE
        # 2. 内部实现需要访问 PCB (xtcp_pcb_t)，这些对用户隐藏
        ${PROJECT_SOURCE_DIR}/src/xnet_tiny/core
        ${PROJECT_SOURCE_DIR}/src/xnet_tiny/transport
)

# 链接底层协议栈
# 这样 xsocket.c 里的 xtcp_send 等函数才能找到实现
target_link_libraries(xnet_socket PRIVATE
        xnet_tiny   # 协议栈核心
        xnet_port   # 适配层 (它会自动带入 driver_pcap 或 driver_dpdk)
)

# =======================================================
# 🚀 核心修改：强制静态链接 (使用 target_link_libraries)
# =======================================================
if(WIN32)
    target_link_libraries(xnet_socket PRIVATE
            # A. 基础运行时强制静态
            -static-libgcc
            -static-libstdc++

            # B. 【关键】开启静态链接模式
            # 告诉链接器：从这里开始，遇到的库都要找 .a (静态库)，不要找 .dll.a
            -Wl,-Bstatic

            # C. 指定要“吃进去”的库
            -lstdc++
            -lwinpthread   # 这就是那个必须吃进去的线程库

            # D. 【关键】恢复动态链接模式
            # 必须恢复！因为后面的 kernel32, user32, ws2_32 等系统库只有动态版
            # 如果不恢复，链接系统库时会报错
            -Wl,-Bdynamic
    )
endif()