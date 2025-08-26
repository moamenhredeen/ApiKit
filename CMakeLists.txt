cmake_minimum_required(VERSION 3.10)

project(apikit VERSION 1.0.0 LANGUAGES C)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Export compile commands for clangd/IDE support
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Project directories
set(SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")
set(INCLUDE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/include")
set(LIBS_DIR "${CMAKE_CURRENT_SOURCE_DIR}/libs")

# Find required packages
find_package(glfw3 REQUIRED)
find_package(CURL REQUIRED)
find_package(OpenGL REQUIRED)

# Source files
set(SOURCES
    ${SRC_DIR}/main.c
    ${SRC_DIR}/http_client.c
    ${SRC_DIR}/http_parser.c
)

# Third-party library sources
set(THIRD_PARTY_SOURCES
    ${LIBS_DIR}/tomlc99/toml.c
)

# Create executable


# Only on macOS: make it a bundle
if(APPLE)
    add_executable(apikit MACOSX_BUNDLE ${SOURCES} ${THIRD_PARTY_SOURCES})
    set_target_properties(apikit PROPERTIES
        MACOSX_BUNDLE TRUE
        MACOSX_BUNDLE_BUNDLE_NAME "APIKIT"
        MACOSX_BUNDLE_GUI_IDENTIFIER "me.moamenhredeen.apikit"
        MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
        #MACOSX_BUNDLE_ICON_FILE "icon.icns"
    )

    set(FONT_FILE ${CMAKE_SOURCE_DIR}/resources/fonts/JetBrainsMonoNL-Regular.ttf)
    set_source_files_properties(${FONT_FILE} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(apikit PRIVATE ${FONT_FILE})
else()
    add_executable(apikit ${SOURCES} ${THIRD_PARTY_SOURCES})
endif()


# Include directories
target_include_directories(apikit PRIVATE
    ${INCLUDE_DIR}                    # Project headers
    ${LIBS_DIR}/nuklear              # Nuklear GUI library
    ${LIBS_DIR}/tomlc99              # TOML configuration library
    ${CURL_INCLUDE_DIR}              # cURL headers
)

# Link libraries
target_link_libraries(apikit PRIVATE 
    glfw 
    ${CURL_LIBRARY}
    ${OPENGL_LIBRARIES}
)

# Compiler definitions for Nuklear
target_compile_definitions(apikit PRIVATE
    NK_INCLUDE_FIXED_TYPES
    NK_INCLUDE_STANDARD_IO
    NK_INCLUDE_STANDARD_VARARGS
    NK_INCLUDE_DEFAULT_ALLOCATOR
    NK_INCLUDE_VERTEX_BUFFER_OUTPUT
    NK_INCLUDE_FONT_BAKING
    NK_INCLUDE_DEFAULT_FONT
    NK_IMPLEMENTATION
    NK_GLFW_GL3_IMPLEMENTATION
)

# Set output directory
set_target_properties(apikit PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

# Debug configuration
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(apikit PRIVATE DEBUG=1)
    target_compile_options(apikit PRIVATE -g -Wall -Wextra)
endif()

# Create documentation target (optional)
find_program(DOXYGEN_EXECUTABLE doxygen)
if(DOXYGEN_EXECUTABLE)
    add_custom_target(docs
        COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMENT "Generating documentation with Doxygen"
    )
endif()

file(COPY resources DESTINATION ${CMAKE_BINARY_DIR})

#-------------------------------------------------------
# Testing Configuration
#-------------------------------------------------------

# Enable testing
enable_testing()

# Test directories
set(TEST_DIR "${CMAKE_CURRENT_SOURCE_DIR}/tests")
set(TEST_UNITY_DIR "${TEST_DIR}/unity")

# Unity test framework sources
set(UNITY_SOURCES
    ${TEST_UNITY_DIR}/unity.c
)

# Library for shared functionality (exclude main.c)
set(LIB_SOURCES
    ${SRC_DIR}/http_client.c
    ${SRC_DIR}/http_parser.c
)

# Create a library for testing (without main.c)
add_library(apikit_lib STATIC ${LIB_SOURCES} ${THIRD_PARTY_SOURCES})

target_include_directories(apikit_lib PUBLIC
    ${INCLUDE_DIR}
    ${LIBS_DIR}/nuklear
    ${LIBS_DIR}/tomlc99
    ${CURL_INCLUDE_DIR}
)

target_link_libraries(apikit_lib PUBLIC 
    ${CURL_LIBRARY}
)

# Test executable for HTTP parser
add_executable(test_http_parser
    ${TEST_DIR}/test_http_parser.c
    ${UNITY_SOURCES}
)

target_include_directories(test_http_parser PRIVATE
    ${INCLUDE_DIR}
    ${TEST_DIR}
)

target_link_libraries(test_http_parser PRIVATE
    apikit_lib
)

# Test executable for HTTP client
add_executable(test_http_client
    ${TEST_DIR}/test_http_client.c
    ${UNITY_SOURCES}
)

target_include_directories(test_http_client PRIVATE
    ${INCLUDE_DIR}
    ${TEST_DIR}
)

target_link_libraries(test_http_client PRIVATE
    apikit_lib
    pthread  # Required for threading in HTTP client tests
)

# Simple HTTP client test (without mock server)
add_executable(test_simple_client
    ${TEST_DIR}/test_simple_client.c
    ${UNITY_SOURCES}
)

target_include_directories(test_simple_client PRIVATE
    ${INCLUDE_DIR}
    ${TEST_DIR}
)

target_link_libraries(test_simple_client PRIVATE
    apikit_lib
)

# Add tests to CTest
add_test(NAME HttpParserTests COMMAND test_http_parser)
add_test(NAME HttpClientTests COMMAND test_http_client)
add_test(NAME SimpleClientTests COMMAND test_simple_client)

# Set test properties
set_tests_properties(HttpParserTests PROPERTIES
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    TIMEOUT 30
)

set_tests_properties(HttpClientTests PROPERTIES
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    TIMEOUT 60
)

set_tests_properties(SimpleClientTests PROPERTIES
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    TIMEOUT 30
)





# Test data setup
add_custom_command(TARGET test_http_parser POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${TEST_DIR}/fixtures ${CMAKE_BINARY_DIR}/tests/fixtures
    COMMENT "Copying test fixtures"
)

add_custom_command(TARGET test_http_client POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory
    ${CMAKE_BINARY_DIR}/tests/output
    COMMENT "Creating test output directory"
)

