#pragma once

#include <filesystem>

#include "TensorRtEngineBuilder.hpp"

namespace ds
{
class TensorRtEngineStore
{
public:
    explicit TensorRtEngineStore(TensorRtEngineBuilder builder);

    std::filesystem::path ensure_engine_file(const std::filesystem::path &onnx_model_path) const;

private:
    TensorRtEngineBuilder builder_;
};
} // namespace ds
