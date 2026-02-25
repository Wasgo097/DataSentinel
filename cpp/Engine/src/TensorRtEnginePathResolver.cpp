#include "TensorRtEnginePathResolver.hpp"

#include <stdexcept>

namespace ds
{
std::filesystem::path TensorRtEnginePathResolver::resolve_engine_path(const std::filesystem::path &onnx_model_path)
{
    if (onnx_model_path.empty())
    {
        throw std::runtime_error("ONNX model path is empty");
    }

    return onnx_model_path.parent_path() / (onnx_model_path.stem().string() + ".engine");
}
} // namespace ds
