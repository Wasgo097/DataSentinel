#include "TensorRtInferenceBackend.hpp"

#include <stdexcept>

namespace ds
{
TensorRtInferenceBackend::TensorRtInferenceBackend(const std::string &model_path)
{
    (void)model_path;
    throw std::runtime_error("TensorRT backend is not implemented yet");
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
    std::ignore = input;
    throw std::runtime_error("TensorRT backend is not implemented yet");
}
} // namespace ds
