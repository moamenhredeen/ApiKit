cmake_minimum_required(VERSION 3.10)

project(apikit)

set(CMAKE_C_STANDARD 99)

# Find required packages
find_package(glfw3 REQUIRED)
find_package(CURL REQUIRED)
find_package(OpenGL REQUIRED)

# Add executable with all source files
add_executable(apikit
    main.c
    http_client.c
    http_parser.c
)

# Link libraries
target_link_libraries(apikit PRIVATE 
    glfw 
    ${CURL_LIBRARY}
    ${OPENGL_LIBRARIES}
)

# Include directories
target_include_directories(apikit PRIVATE 
    ${CURL_INCLUDE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}  # For nuklear.h
)

# Nuklear definitions
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
