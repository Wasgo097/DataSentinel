cmake_minimum_required(VERSION 3.20)
project(DataSentinelReceiver)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
option(DS_ENABLE_TENSORRT "Enable TensorRT backend support" OFF)
option(protobuf_MODULE_COMPATIBLE TRUE)

if(DS_ENABLE_TENSORRT)
    message(STATUS "DataSentinel Engine backend build: ONNX + TensorRT")
else()
    message(STATUS "DataSentinel Engine backend build: ONNX only")
endif()

find_package(Boost REQUIRED COMPONENTS system)
find_package(Threads REQUIRED)

# Resolve Protobuf toolchain/library.
find_package(Protobuf CONFIG QUIET)
if(Protobuf_FOUND)
    set(DS_PROTOBUF_LIB protobuf::libprotobuf)
    if(CMAKE_CROSSCOMPILING)
        find_program(DS_PROTOC protoc REQUIRED)
    else()
        set(DS_PROTOC $<TARGET_FILE:protobuf::protoc>)
    endif()
else()
    find_package(Protobuf REQUIRED)
    set(DS_PROTOBUF_LIB ${Protobuf_LIBRARIES})
    if(DEFINED Protobuf_PROTOC_EXECUTABLE)
        set(DS_PROTOC ${Protobuf_PROTOC_EXECUTABLE})
    else()
        find_program(DS_PROTOC protoc REQUIRED)
    endif()
endif()

# gRPC CMake package (installed system-wide or custom prefix).
find_package(gRPC CONFIG REQUIRED)
set(DS_GRPCPP gRPC::grpc++)
if(CMAKE_CROSSCOMPILING)
    find_program(DS_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin REQUIRED)
else()
    if(TARGET gRPC::grpc_cpp_plugin)
        set(DS_GRPC_CPP_PLUGIN_EXECUTABLE $<TARGET_FILE:gRPC::grpc_cpp_plugin>)
    else()
        find_program(DS_GRPC_CPP_PLUGIN_EXECUTABLE grpc_cpp_plugin REQUIRED)
    endif()
endif()

# Shared proto contract used by engine and producer.
get_filename_component(ds_proto "../../proto/inference.proto" ABSOLUTE)
get_filename_component(ds_proto_path "${ds_proto}" PATH)

set(ds_proto_src "${CMAKE_CURRENT_BINARY_DIR}/inference.pb.cc")
set(ds_proto_hdr "${CMAKE_CURRENT_BINARY_DIR}/inference.pb.h")
set(ds_grpc_src "${CMAKE_CURRENT_BINARY_DIR}/inference.grpc.pb.cc")
set(ds_grpc_hdr "${CMAKE_CURRENT_BINARY_DIR}/inference.grpc.pb.h")

add_custom_command(
    OUTPUT "${ds_proto_src}" "${ds_proto_hdr}" "${ds_grpc_src}" "${ds_grpc_hdr}"
    COMMAND ${DS_PROTOC}
    ARGS --grpc_out "${CMAKE_CURRENT_BINARY_DIR}"
         --cpp_out "${CMAKE_CURRENT_BINARY_DIR}"
         -I "${ds_proto_path}"
         --plugin=protoc-gen-grpc="${DS_GRPC_CPP_PLUGIN_EXECUTABLE}"
         "${ds_proto}"
    DEPENDS "${ds_proto}"
)

add_library(ds_grpc_proto
    ${ds_proto_src}
    ${ds_proto_hdr}
    ${ds_grpc_src}
    ${ds_grpc_hdr}
)
target_link_libraries(ds_grpc_proto
    PUBLIC
        ${DS_GRPCPP}
        ${DS_PROTOBUF_LIB}
        Threads::Threads
)
target_include_directories(ds_grpc_proto
    PUBLIC
        "${CMAKE_CURRENT_BINARY_DIR}"
)
if(NOT TARGET protobuf::libprotobuf)
    target_include_directories(ds_grpc_proto PUBLIC ${Protobuf_INCLUDE_DIRS})
endif()

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
    src/GrpcServer.cpp
    src/InferenceBackendFactory.cpp
    src/InputParser.cpp
    src/OnnxInferenceBackend.cpp
    src/TcpServer.cpp
    src/TensorRtEngineBuilder.cpp
    src/TensorRtEnginePathResolver.cpp
    src/TensorRtEngineStore.cpp
    src/TensorRtInferenceBackend.cpp
)

target_include_directories(${PROJECT_NAME}
    PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_BINARY_DIR}"
)

target_link_libraries(${PROJECT_NAME}
    Boost::system
    onnxruntime::onnxruntime
    ds_grpc_proto
)

if(DS_ENABLE_TENSORRT)
    find_package(CUDAToolkit REQUIRED)

    find_path(TENSORRT_INCLUDE_DIR NvInfer.h
        PATHS
            /usr/include/x86_64-linux-gnu
            /usr/include
            /usr/local/include
            /usr/local/TensorRT/include
        REQUIRED
    )

    find_library(TENSORRT_NVINFER_LIB nvinfer
        PATHS
            /usr/lib/x86_64-linux-gnu
            /usr/lib
            /usr/local/lib
            /usr/local/TensorRT/lib
        REQUIRED
    )

    find_library(TENSORRT_NVONNXPARSER_LIB nvonnxparser
        PATHS
            /usr/lib/x86_64-linux-gnu
            /usr/lib
            /usr/local/lib
            /usr/local/TensorRT/lib
        REQUIRED
    )

    target_compile_definitions(${PROJECT_NAME} PRIVATE DS_ENABLE_TENSORRT=1)
    target_include_directories(${PROJECT_NAME} PRIVATE "${TENSORRT_INCLUDE_DIR}")
    target_link_libraries(${PROJECT_NAME}
        CUDA::cudart
        "${TENSORRT_NVINFER_LIB}"
        "${TENSORRT_NVONNXPARSER_LIB}"
    )
else()
    target_compile_definitions(${PROJECT_NAME} PRIVATE DS_ENABLE_TENSORRT=0)
endif()
