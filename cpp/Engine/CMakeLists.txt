cmake_minimum_required(VERSION 3.20)
project(DataSentinelReceiver)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Boost REQUIRED COMPONENTS system)

# Find ONNX Runtime (expects the package to provide a CMake config).
# Priority:
# 1) explicit -Donnxruntime_DIR=...
# 2) ONNXRUNTIME_DIR environment variable
# 3) auto-detect under $HOME/onnxruntime-linux-x64-*/(lib64|lib)/cmake/onnxruntime
if(NOT DEFINED onnxruntime_DIR)
    if(DEFINED ENV{ONNXRUNTIME_DIR})
        set(onnxruntime_DIR "$ENV{ONNXRUNTIME_DIR}")
    else()
        file(GLOB _ORT_CANDIDATES
            "$ENV{HOME}/onnxruntime-linux-x64-*/lib64/cmake/onnxruntime"
            "$ENV{HOME}/onnxruntime-linux-x64-*/lib/cmake/onnxruntime"
        )
        list(LENGTH _ORT_CANDIDATES _ORT_CANDIDATES_COUNT)
        if(_ORT_CANDIDATES_COUNT GREATER 0)
            list(SORT _ORT_CANDIDATES)
            list(REVERSE _ORT_CANDIDATES)
            list(GET _ORT_CANDIDATES 0 onnxruntime_DIR)
        endif()
    endif()
endif()
find_package(onnxruntime REQUIRED CONFIG)

add_executable(${PROJECT_NAME}
    main.cpp
    src/AnomalyDetector.cpp
    src/ClientSession.cpp
    src/ConfigLoader.cpp
    src/InferenceBackendFactory.cpp
    src/InputParser.cpp
    src/OnnxInferenceBackend.cpp
    src/TcpServer.cpp
    src/TensorRtInferenceBackend.cpp
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(${PROJECT_NAME}
    Boost::system
    onnxruntime::onnxruntime
)
