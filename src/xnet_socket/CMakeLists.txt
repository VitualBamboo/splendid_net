# src/xnet_socket/CMakeLists.txt

set(SOCKET_SRCS
        xsocket.c
)

add_library(xnet_socket STATIC ${SOCKET_SRCS})

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
target_link_libraries(xnet_socket PRIVATE xnet_tiny)