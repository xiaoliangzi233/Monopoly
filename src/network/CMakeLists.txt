qt_add_library(neon_network STATIC
    protocol.h
    protocol.cpp
    host_server.h
    host_server.cpp
    game_client.h
    game_client.cpp
    lan_discovery.h
    lan_discovery.cpp
)

target_include_directories(neon_network PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(neon_network PUBLIC neon_core Qt6::Core Qt6::Network)
