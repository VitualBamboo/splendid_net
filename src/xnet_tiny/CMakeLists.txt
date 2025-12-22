# 扫描当前目录下的.c文件（并监控文件变动，有新增时自动刷新）
file(GLOB TINY_SRCS CONFIGURE_DEPENDS "*.c")

# 新建一个xnet_tiny库，存放.c编译后的.o
add_library(xnet_tiny OBJECT ${TINY_SRCS})

# 管理xnet_tiny库的头文件路径
target_include_directories(xnet_tiny
        PUBLIC
        # 对外，宣布自己的头文件路径
        ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE
        # 对内，需要调用 Port 层的接口
        ${PROJECT_SOURCE_DIR}/src/xnet_port
)