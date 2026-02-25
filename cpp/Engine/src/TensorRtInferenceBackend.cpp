#include "TensorRtInferenceBackend.hpp"

#include <stdexcept>

namespace ds
{
TensorRtInferenceBackend::TensorRtInferenceBackend(const std::string &model_path)
{
    std::ignore = model_path;
    throw std::runtime_error("TensorRT backend is not implemented yet for this build");
}

std::string TensorRtInferenceBackend::backend_name() const
{
    return "tensorrt";
}

std::size_t TensorRtInferenceBackend::expected_input_size() const
{
    throw std::runtime_error("TensorRT backend is not implemented yet");
}

std::vector<float> TensorRtInferenceBackend::reconstruct(const std::vector<float> &input)
{
    (void)input;
    throw std::runtime_error("TensorRT backend is not implemented yet for this build");
}
} // namespace ds
