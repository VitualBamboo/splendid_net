cmake_minimum_required(VERSION 3.16)
project(splendid_net C)

set(CMAKE_C_STANDARD 99)

# 1. 包含通用路径 (Windows/Linux 都需要的)
include_directories(
        ${PROJECT_SOURCE_DIR}
        ${PROJECT_SOURCE_DIR}/src/xnet_app
        ${PROJECT_SOURCE_DIR}/src/xnet_tiny
        ${PROJECT_SOURCE_DIR}/lib/xnet
)

# 2. 针对 Windows 的特殊配置
if(WIN32)
    # Windows 下 Npcap SDK 路径
    set(NPCAP_SDK_DIR "C:/Users/efair/Downloads/npcap-sdk-1.15")

    # 添加 Windows 特有的头文件路径
    include_directories(${NPCAP_SDK_DIR}/Include)

    # Npcap DLL 自动复制脚本 (仅在 Windows 生效)
    set(NPCAP_DLL_DIR "$ENV{SystemRoot}/System32/Npcap")
    set(NPCAP_RUNTIME_DLLS
            "${NPCAP_DLL_DIR}/wpcap.dll"
            "${NPCAP_DLL_DIR}/Packet.dll"
    )
    # 注意：add_custom_command 必须放在 add_executable 之后，所以这里先定义变量
endif()

# 3. 生成可执行文件
add_executable(${PROJECT_NAME} lib/xnet/pcap_device.c src/app.c)

# 4. 后处理与链接 (核心修改部分)

# 添加子模块 (通用)
add_subdirectory(src/xnet_tiny)
add_subdirectory(src/xnet_app)

if(WIN32)
    # -------- Windows 链接逻辑 --------
    # 链接 xnet 模块 + Npcap SDK + Winsock
    target_link_libraries(${PROJECT_NAME} PRIVATE
            xnet_tiny
            xnet_app
            ${NPCAP_SDK_DIR}/Lib/x64/wpcap.lib
            Ws2_32
    )

    # 执行 DLL 复制命令
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different ${NPCAP_RUNTIME_DLLS} $<TARGET_FILE_DIR:${PROJECT_NAME}>
            COMMENT "Copying Npcap runtime DLLs to target directory..."
    )

else()
    # -------- Linux (Ubuntu) 链接逻辑 --------
    # Linux 下不需要指定 SDK 路径，系统库直接能找到
    # 链接 xnet 模块 + pcap (系统库) + pthread (线程库)
    target_link_libraries(${PROJECT_NAME} PRIVATE
            xnet_tiny
            xnet_app
            pcap
            pthread
    )
endif()
