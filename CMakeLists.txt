cmake_minimum_required(VERSION 4.0)
project(LanternImageLoader LANGUAGES CXX CUDA)
list(APPEND CMAKE_PREFIX_PATH D:/module)

include_directories(${CMAKE_PREFIX_PATH}/include)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CUDA_STANDARD 17)
set(CMAKE_CUDA_STANDARD_REQUIRED ON)
set(CMAKE_CUDA_SEPARABLE_COMPILATION ON)
set(CMAKE_CUDA_LINK_EXECUTABLE ON)
set(CMAKE_CUDA_ARCHITECTURES 86)


find_package(ArrayFire REQUIRED)

# Get all header files in the current directory
file(
    GLOB header_content
    "${CMAKE_SOURCE_DIR}/headers/**/*.h"
    "${CMAKE_SOURCE_DIR}/headers/*.h"
)

file(
    GLOB source_content
    "${CMAKE_SOURCE_DIR}/src/**/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_SOURCE_DIR}/src/**/*.cu"
    "${CMAKE_SOURCE_DIR}/src/*.cu"
)

add_executable(${PROJECT_NAME} ${source_content} ${header_content} pch.h)
set_target_properties(${PROJECT_NAME} PROPERTIES CUDA_RESOLVE_DEVICE_SYMBOLS ON)
target_link_libraries(${PROJECT_NAME} PRIVATE ArrayFire::afcuda)
target_precompile_headers(${PROJECT_NAME} PUBLIC pch.h)
