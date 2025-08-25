cmake_minimum_required(VERSION 3.10)

project(apikit)

set(CMAKE_C_STANDARD 99)

find_package(glfw3 REQUIRED)
if(NOT glfw3_FOUND)
    message(FATAL_ERROR "GLFW not found. Install with: brew install glfw")
endif()

add_executable(apikit main.c http_client.c)

target_link_libraries(apikit PRIVATE glfw)
