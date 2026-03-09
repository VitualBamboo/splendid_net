# src/xnet_tiny/arch/CMakeLists.txt

set(ARCH_SRCS "")

# 根据不同操作系统，精准挂载对应的系统时间摇把
if(WIN32)
    list(APPEND ARCH_SRCS arch_sys_win32.c)
else()
    list(APPEND ARCH_SRCS arch_sys_linux.c)
endif()

# 编译为一个独立的静态库
add_library(xnet_arch STATIC ${ARCH_SRCS})

# 包含 core 目录，因为 arch_sys_xxx.c 需要实现 xnet_tiny.h 里的 xsys_get_time 契约
target_include_directories(xnet_arch
        PRIVATE
        ../core
)