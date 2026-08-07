cmake_minimum_required(VERSION 3.24)
if(POLICY CMP0207)
    cmake_policy(SET CMP0207 NEW)
endif()

project(NeonTycoon VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(QT_QML_GENERATE_QMLLS_INI ON)

option(NEON_BUILD_TESTS "Build Neon Tycoon tests" ON)
option(NEON_BUILD_LAUNCHER "Build the standalone launcher" ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS Core Gui Network Qml Quick QuickControls2)

qt_standard_project_setup()
if(COMMAND qt_policy)
    qt_policy(SET QTP0001 NEW)
    qt_policy(SET QTP0004 NEW)
endif()

add_subdirectory(src/core)
add_subdirectory(src/network)
add_subdirectory(src/app)

if(NEON_BUILD_LAUNCHER)
    add_subdirectory(src/launcher)
endif()

if(NEON_BUILD_TESTS)
    enable_testing()
    find_package(Qt6 6.4 REQUIRED COMPONENTS Test)
    add_subdirectory(tests)
endif()

install(FILES LICENSE README.md THIRD_PARTY_NOTICES.md DESTINATION licenses)

set(NEON_QT_LICENSE_PATHS
    "${CMAKE_PREFIX_PATH}/LICENSES"
    "${CMAKE_PREFIX_PATH}/share/licenses/qt6-base"
    "${Qt6_DIR}/../../../LICENSES"
    "${Qt6_DIR}/../../../share/licenses/qt6-base"
)
find_file(NEON_QT_LGPL3 NAMES LGPL-3.0-only.txt PATHS ${NEON_QT_LICENSE_PATHS} NO_DEFAULT_PATH)
find_file(NEON_QT_LGPL21 NAMES LGPL-2.1-or-later.txt PATHS ${NEON_QT_LICENSE_PATHS} NO_DEFAULT_PATH)
if(NEON_QT_LGPL3)
    install(FILES "${NEON_QT_LGPL3}" DESTINATION licenses RENAME Qt-LGPL-3.0-only.txt)
else()
    message(WARNING "Qt LGPL-3.0 license text was not found; production packaging must provide it")
endif()
if(NEON_QT_LGPL21)
    install(FILES "${NEON_QT_LGPL21}" DESTINATION licenses RENAME Qt-LGPL-2.1-or-later.txt)
endif()

install(RUNTIME_DEPENDENCY_SET neon_runtime_dependencies
    DESTINATION bin
    DIRECTORIES ${CMAKE_PREFIX_PATH}/bin
    PRE_EXCLUDE_REGEXES "api-ms-.*" "ext-ms-.*"
    POST_EXCLUDE_REGEXES ".*[Ss]ystem32/.*" ".*[Ww]indows/[Ss]ysWOW64/.*"
)

set(CPACK_PACKAGE_NAME "Neon Tycoon")
set(CPACK_PACKAGE_VENDOR "Neon Tycoon Contributors")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_INSTALL_DIRECTORY "Neon Tycoon")
set(CPACK_NSIS_DISPLAY_NAME "Neon Tycoon")
set(CPACK_NSIS_PACKAGE_NAME "Neon Tycoon")
set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
set(CPACK_NSIS_MUI_FINISHPAGE_RUN "bin/NeonTycoonLauncher.exe")
include(CPack)
