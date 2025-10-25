cmake_minimum_required(VERSION 3.16)
project(splendid_net C)

set(CMAKE_C_STANDARD 99)

# 设置 npcap sdk 的路径（请根据你实际安装路径修改）
set(NPCAP_SDK_DIR "C:/Users/efair/Downloads/npcap-sdk-1.15")

# 添加头文件搜索路径
include_directories(
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/src/xnet_app
        ${PROJECT_SOURCE_DIR}/src/xnet_tiny
        ${PROJECT_SOURCE_DIR}/lib/xnet
        ${NPCAP_SDK_DIR}/Include
)

# 生成一个可执行文件，名字是 splendid_net
add_executable(${PROJECT_NAME} lib/xnet/pcap_device.c src/app.c)

# ==========================================================
# 自动部署 Npcap 运行时 DLL (新增部分)
# ==========================================================
if(WIN32)
    # 1. 定义 Npcap 运行时 DLL 的源目录
    # 根据你确认的信息，DLL 位于 C:\Windows\System32\Npcap
    set(NPCAP_DLL_DIR "$ENV{SystemRoot}/System32/Npcap")

    # 2. 定义需要复制的文件列表
    set(NPCAP_RUNTIME_DLLS
            "${NPCAP_DLL_DIR}/wpcap.dll"
            "${NPCAP_DLL_DIR}/Packet.dll"
    )

    # 3. 添加 POST_BUILD 命令：在编译完成后执行复制操作
    # $<TARGET_FILE_DIR:${PROJECT_NAME}> 是指你的可执行文件 (splendid_net.exe) 所在的目录
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${NPCAP_RUNTIME_DLLS} $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMENT "Copying Npcap runtime DLLs to target directory..."
    )
endif()
# ==========================================================


# 添加子模块
add_subdirectory(src/xnet_tiny)
add_subdirectory(src/xnet_app)

# 把一些库链接到最终的可执行文件中
target_link_libraries(${PROJECT_NAME} PRIVATE xnet_tiny xnet_app ${NPCAP_SDK_DIR}/Lib/x64/wpcap.lib Ws2_32)
