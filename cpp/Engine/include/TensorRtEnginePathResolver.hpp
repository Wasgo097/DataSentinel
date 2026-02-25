#pragma once

#include <filesystem>

namespace ds
{
class TensorRtEnginePathResolver
{
public:
    static std::filesystem::path resolve_engine_path(const std::filesystem::path &onnx_model_path);
};
} // namespace ds
