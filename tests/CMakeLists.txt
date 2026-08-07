add_executable(neon_core_tests core_tests.cpp)
target_include_directories(neon_core_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(neon_core_tests PRIVATE neon_core neon_network Qt6::Core Qt6::Test)
add_test(NAME neon_core_tests COMMAND neon_core_tests)
get_target_property(NEON_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
get_filename_component(NEON_QT_BIN_DIRECTORY "${NEON_QMAKE_EXECUTABLE}" DIRECTORY)
set_tests_properties(neon_core_tests PROPERTIES
    TIMEOUT 180
    ENVIRONMENT_MODIFICATION "PATH=path_list_prepend:${NEON_QT_BIN_DIRECTORY}"
)
