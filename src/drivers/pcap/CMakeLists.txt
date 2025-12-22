# 1. 定义一个静态库 (driver_pcap)
# STATIC 意味着它会生成 driver_pcap.lib (Windows) 或 .a (Linux)
add_library(driver_pcap STATIC pcap_device.c)

# 2. 它是怎么干活的？它需要依赖 Npcap::SDK
target_link_libraries(driver_pcap PUBLIC Npcap::SDK)

# 3. 对外公布头文件路径
# 这句是精华：谁链接了我，谁就能自动获得我的头文件路径
target_include_directories(driver_pcap PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})