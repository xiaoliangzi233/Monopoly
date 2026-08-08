qt_add_executable(neon_tycoon WIN32
    main.cpp
    game_view_model.h
    game_view_model.cpp
    procedural_audio.h
    procedural_audio.cpp
    release_notes.h
    release_notes.cpp
    top_down_city_view.h
    top_down_city_view.cpp
)

option(NEON_CONSOLE_DIAGNOSTICS "Build the game as a console executable for local QML diagnostics" OFF)
if(NEON_CONSOLE_DIAGNOSTICS)
    set_target_properties(neon_tycoon PROPERTIES WIN32_EXECUTABLE FALSE)
endif()

qt_add_qml_module(neon_tycoon
    URI NeonTycoon
    VERSION 1.0
    QML_FILES
        qml/Main.qml
        qml/ModernButton.qml
        qml/ModernModal.qml
        qml/ModernStepper.qml
        qml/SegmentedChoice.qml
    RESOURCES
        resources/release_notes.json
)

target_include_directories(neon_tycoon PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(neon_tycoon PRIVATE neon_core neon_network
    Qt6::Core Qt6::Gui Qt6::Qml Qt6::Quick Qt6::QuickControls2)
if(TARGET Qt6::Multimedia)
    target_link_libraries(neon_tycoon PRIVATE Qt6::Multimedia)
    target_compile_definitions(neon_tycoon PRIVATE NEON_HAVE_MULTIMEDIA=1)
endif()
set_target_properties(neon_tycoon PROPERTIES OUTPUT_NAME NeonTycoon)

include(GNUInstallDirs)
install(TARGETS neon_tycoon
    RUNTIME_DEPENDENCY_SET neon_runtime_dependencies
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
qt_generate_deploy_qml_app_script(
    TARGET neon_tycoon
    OUTPUT_SCRIPT NEON_GAME_DEPLOY_SCRIPT
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${NEON_GAME_DEPLOY_SCRIPT})
