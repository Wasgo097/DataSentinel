cmake_minimum_required(VERSION 3.20)
project(DataSentinelReceiver)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS system)

add_executable(${PROJECT_NAME} main.cpp)

# Find ONNX Runtime (expects the package to provide a CMake config)
set(onnxruntime_DIR "$ENV{HOME}/onnxruntime-linux-x64-1.24.2/lib64/cmake/onnxruntime")
find_package(onnxruntime REQUIRED CONFIG)

target_link_libraries(${PROJECT_NAME}
    Boost::system
    onnxruntime::onnxruntime
)

# Ensure the header-only json.hpp in cpp/ is visible to the target and included in project sources
target_include_directories(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/cpp")
target_sources(${PROJECT_NAME} PRIVATE "${CMAKE_SOURCE_DIR}/json.hpp")