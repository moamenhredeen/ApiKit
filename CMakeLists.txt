cmake_minimum_required(VERSION 3.10)

project(apikit)

set(CMAKE_C_STANDARD 99)

find_package(glfw3 REQUIRED)
find_package(CURL REQUIRED)

add_executable(apikit main.c http_client.c)

target_link_libraries(apikit PRIVATE glfw ${CURL_LIBRARY})
target_include_directories(apikit PRIVATE ${CURL_INCLUDE_DIR})
