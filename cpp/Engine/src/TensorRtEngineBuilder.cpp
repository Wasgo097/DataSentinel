#include "TensorRtEngineBuilder.hpp"

#include <fstream>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <utility>

#if DS_ENABLE_TENSORRT
#include <NvInfer.h>
#include <NvOnnxParser.h>
#endif

namespace ds
{
TensorRtEngineBuilder::TensorRtEngineBuilder(TensorRtBuildOptions options)
    : options_(std::move(options))
{
}

std::vector<char> TensorRtEngineBuilder::build_serialized_engine(const std::filesystem::path &onnx_model_path) const
{
#if DS_ENABLE_TENSORRT
    if (!std::filesystem::exists(onnx_model_path))
    {
        throw std::runtime_error("ONNX model not found: " + onnx_model_path.string());
    }

    class TensorRtLogger : public nvinfer1::ILogger
    {
    public:
        void log(Severity severity, const char *msg) noexcept override
        {
            if (severity <= Severity::kWARNING)
            {
                std::cerr << "[TensorRT] " << msg << '\n';
            }
        }
    };

    TensorRtLogger logger;

    auto builder = std::unique_ptr<nvinfer1::IBuilder>(nvinfer1::createInferBuilder(logger));
    if (!builder)
    {
        throw std::runtime_error("Failed to create TensorRT builder");
    }

    const auto explicit_batch_flag =
        1U << static_cast<unsigned>(nvinfer1::NetworkDefinitionCreationFlag::kEXPLICIT_BATCH);
    auto network = std::unique_ptr<nvinfer1::INetworkDefinition>(builder->createNetworkV2(explicit_batch_flag));
    if (!network)
    {
        throw std::runtime_error("Failed to create TensorRT network definition");
    }

    auto parser = std::unique_ptr<nvonnxparser::IParser>(nvonnxparser::createParser(*network, logger));
    if (!parser)
    {
        throw std::runtime_error("Failed to create TensorRT ONNX parser");
    }

    const bool parsed =
        parser->parseFromFile(onnx_model_path.c_str(), static_cast<int>(nvinfer1::ILogger::Severity::kWARNING));
    if (!parsed)
    {
        throw std::runtime_error("Failed to parse ONNX file with TensorRT: " + onnx_model_path.string());
    }

    auto config = std::unique_ptr<nvinfer1::IBuilderConfig>(builder->createBuilderConfig());
    if (!config)
    {
        throw std::runtime_error("Failed to create TensorRT builder config");
    }

    config->setMemoryPoolLimit(nvinfer1::MemoryPoolType::kWORKSPACE, options_.max_workspace_size_bytes);

    if (options_.enable_fp16 && builder->platformHasFastFp16())
    {
        config->setFlag(nvinfer1::BuilderFlag::kFP16);
    }

    auto serialized_engine = std::unique_ptr<nvinfer1::IHostMemory>(builder->buildSerializedNetwork(*network, *config));
    if (!serialized_engine)
    {
        throw std::runtime_error("Failed to build serialized TensorRT engine");
    }

    const auto *bytes = static_cast<const char *>(serialized_engine->data());
    const std::size_t size = serialized_engine->size();
    return std::vector<char>(bytes, bytes + size);
#else
    (void)onnx_model_path;
    throw std::runtime_error("TensorRT builder is unavailable because DS_ENABLE_TENSORRT=OFF");
#endif
}
} // namespace ds
