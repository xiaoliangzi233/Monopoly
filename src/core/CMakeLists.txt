qt_add_library(neon_core STATIC
    game_types.h
    game_state.h
    game_state.cpp
    game_engine.h
    game_engine.cpp
    city_content.h
    city_content.cpp
    ai_player.h
    ai_player.cpp
    save_manager.h
    save_manager.cpp
    scene_layout.h
    scene_layout.cpp
)

target_include_directories(neon_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(neon_core PUBLIC Qt6::Core Qt6::Gui)
target_compile_definitions(neon_core PUBLIC NEON_VERSION="${PROJECT_VERSION}")
