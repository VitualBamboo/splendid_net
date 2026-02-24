cmake_minimum_required(VERSION 3.16)
project(splendid_net C)

# =======================================================
# 🌟 终极补丁：强制所有代码生成“位置无关代码” (-fPIC)
# 这是 Linux 下把静态库揉进 .so 的唯一合法通行证！
# =======================================================
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_C_STANDARD 99)

# =======================================================
# 1. 弹药库输出路径配置 (保证生成的 .so 和 .dll 都在根目录下)
# =======================================================
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

if(NOT WIN32)
    add_compile_options(-march=native)
endif()

# =======================================================
# 2. 跨平台网卡驱动配置：Windows(Npcap) vs Linux(DPDK)
# =======================================================
if(WIN32)
    # --- Windows Npcap 配置 ---
    set(NPCAP_SDK_DIR "C:/Users/efair/Downloads/npcap-sdk-1.15")

    add_library(Npcap::SDK UNKNOWN IMPORTED)
    set_target_properties(Npcap::SDK PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${NPCAP_SDK_DIR}/Include"
            IMPORTED_LOCATION "${NPCAP_SDK_DIR}/Lib/x64/wpcap.lib"
            INTERFACE_LINK_LIBRARIES "${NPCAP_SDK_DIR}/Lib/x64/Packet.lib;Ws2_32"
    )
else()
    # --- Linux DPDK 配置 ---
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(DPDK REQUIRED libdpdk)

    add_library(DPDK::SDK INTERFACE IMPORTED)
    target_include_directories(DPDK::SDK INTERFACE ${DPDK_INCLUDE_DIRS})
    target_link_directories(DPDK::SDK INTERFACE ${DPDK_LIBRARY_DIRS})

    # 🌟 DPDK 核武器绑定：强制打包所有驱动，防止网卡失联
    target_link_libraries(DPDK::SDK INTERFACE
            -Wl,--whole-archive ${DPDK_LIBRARIES} -Wl,--no-whole-archive
            pthread numa dl m
    )
    add_compile_definitions(ALLOW_EXPERIMENTAL_API)
endif()

# =======================================================
# 3. 注册协议栈核心模块
# =======================================================
if(WIN32)
    add_subdirectory(src/drivers/pcap)
else()
    add_subdirectory(src/drivers/dpdk)
endif()

add_subdirectory(src/xnet_port)
add_subdirectory(src/xnet_tiny)
add_subdirectory(src/xnet_socket)
add_subdirectory(src/xnet_app)

# =======================================================
# 🌟 4. 核心魔法：为 JNA 专用的动态库注入灵魂 (.so / .dll)
# =======================================================
if(TARGET xnet_socket)
    if(WIN32)
        # Windows下：把 Npcap 驱动和核心协议栈绑进 DLL
        target_link_libraries(xnet_socket PRIVATE xnet_tiny xnet_port Npcap::SDK)
    else()
        # Linux下：把 DPDK 驱动和核心协议栈绑进 SO
        target_link_libraries(xnet_socket PRIVATE xnet_tiny xnet_port DPDK::SDK)
    endif()
endif()

# =======================================================
# 5. 生成主测试程序 (splendid_net.exe / splendid_net)
# =======================================================
add_executable(${PROJECT_NAME}
        src/app.c
        src/xnet_socket/xsocket.c
)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src/xnet_tiny/core
        ${CMAKE_CURRENT_SOURCE_DIR}/src/xnet_tiny/transport
        ${CMAKE_CURRENT_SOURCE_DIR}/src/xnet_socket
        ${CMAKE_CURRENT_SOURCE_DIR}/src/xnet_app
)

if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
            xnet_app xnet_tiny xnet_port Npcap::SDK
    )
else()
    target_link_libraries(${PROJECT_NAME} PRIVATE
            xnet_app xnet_tiny xnet_port DPDK::SDK
    )
endif()