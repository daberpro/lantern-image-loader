cmake_minimum_required(VERSION 4.0)
project(LanternImageLoader LANGUAGES CXX)

list(APPEND CMAKE_PREFIX_PATH D:/module)
include(config/cpp_cuda.cmake)

add_library(${PROJECT_NAME} STATIC main.cu header.h)
add_executable(LanternTest main.cpp)
target_link_libraries(
    LanternTest PRIVATE
    ${PROJECT_NAME}
)