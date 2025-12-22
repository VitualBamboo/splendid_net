set(APP_SRCS
        xserver_datetime.c
        xserver_http.c
)

add_library(xnet_app OBJECT ${APP_SRCS})

target_include_directories(xnet_app
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        PRIVATE
        ${PROJECT_SOURCE_DIR}/src/xnet_tiny/core
        ${PROJECT_SOURCE_DIR}/src/xnet_tiny/transport
        ${PROJECT_SOURCE_DIR}/src/xnet_tiny/network
)