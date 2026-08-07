find_path(SODIUM_INCLUDE_DIR sodium.h PATHS ${CMAKE_PREFIX_PATH}/include)
find_library(SODIUM_LIBRARY NAMES sodium libsodium PATHS ${CMAKE_PREFIX_PATH}/lib)
find_file(SODIUM_RUNTIME NAMES libsodium-26.dll libsodium.dll sodium.dll PATHS ${CMAKE_PREFIX_PATH}/bin)

set(NEON_UPDATE_PUBLIC_KEY_HEX "" CACHE STRING "Ed25519 update public key as 64 hexadecimal characters")
set(NEON_GITHUB_MANIFEST_URL "" CACHE STRING "GitHub raw update manifest URL")
set(NEON_GITEE_MANIFEST_URL "" CACHE STRING "Gitee raw update manifest URL")

qt_add_executable(neon_launcher WIN32
    main.cpp
    update_manifest.h
    update_manifest.cpp
    update_controller.h
    update_controller.cpp
)

qt_add_qml_module(neon_launcher
    URI NeonTycoonLauncher
    VERSION 1.0
    QML_FILES qml/Launcher.qml
)

target_include_directories(neon_launcher PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/..)
target_link_libraries(neon_launcher PRIVATE Qt6::Core Qt6::Gui Qt6::Network Qt6::Qml Qt6::Quick Qt6::QuickControls2)
target_compile_definitions(neon_launcher PRIVATE
    NEON_VERSION="${PROJECT_VERSION}"
    NEON_UPDATE_PUBLIC_KEY_HEX="${NEON_UPDATE_PUBLIC_KEY_HEX}"
    NEON_GITHUB_MANIFEST_URL="${NEON_GITHUB_MANIFEST_URL}"
    NEON_GITEE_MANIFEST_URL="${NEON_GITEE_MANIFEST_URL}"
)
if(SODIUM_INCLUDE_DIR AND SODIUM_LIBRARY)
    target_include_directories(neon_launcher PRIVATE ${SODIUM_INCLUDE_DIR})
    target_link_libraries(neon_launcher PRIVATE ${SODIUM_LIBRARY})
    target_compile_definitions(neon_launcher PRIVATE NEON_HAVE_SODIUM=1)
endif()
set_target_properties(neon_launcher PROPERTIES OUTPUT_NAME NeonTycoonLauncher)

include(GNUInstallDirs)
install(TARGETS neon_launcher
    RUNTIME_DEPENDENCY_SET neon_runtime_dependencies
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
if(SODIUM_RUNTIME)
    install(FILES ${SODIUM_RUNTIME} DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()
if(MINGW)
    foreach(NEON_RUNTIME_NAME libgcc_s_seh-1.dll libstdc++-6.dll libwinpthread-1.dll)
        find_file(NEON_RUNTIME_${NEON_RUNTIME_NAME} NAMES ${NEON_RUNTIME_NAME} PATHS ${CMAKE_PREFIX_PATH}/bin)
        if(NEON_RUNTIME_${NEON_RUNTIME_NAME})
            install(FILES ${NEON_RUNTIME_${NEON_RUNTIME_NAME}} DESTINATION ${CMAKE_INSTALL_BINDIR})
        endif()
    endforeach()
endif()
qt_generate_deploy_qml_app_script(
    TARGET neon_launcher
    OUTPUT_SCRIPT NEON_LAUNCHER_DEPLOY_SCRIPT
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${NEON_LAUNCHER_DEPLOY_SCRIPT})
