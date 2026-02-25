#pragma once

#include <filesystem>
#include <vector>

#include "TensorRtBuildOptions.hpp"

namespace ds
{
class TensorRtEngineBuilder
{
public:
    explicit TensorRtEngineBuilder(TensorRtBuildOptions options);

    std::vector<char> build_serialized_engine(const std::filesystem::path &onnx_model_path) const;

private:
    TensorRtBuildOptions options_;
};
} // namespace ds
