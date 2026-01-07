# src/xnet_app/CMakeLists.txt

set(APP_SRCS
        xserver_datetime.c
        xserver_http.c
)

add_library(xnet_app STATIC ${APP_SRCS})

target_include_directories(xnet_app
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE
        ${PROJECT_SOURCE_DIR}/src/xnet_tiny/core
        ${PROJECT_SOURCE_DIR}/src/xnet_socket
)