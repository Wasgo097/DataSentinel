#include "OnnxInferenceBackend.hpp"

#include <filesystem>
#include <stdexcept>

namespace fs = std::filesystem;

namespace ds
{
OnnxInferenceBackend::OnnxInferenceBackend(const std::string &model_path)
    : env_(ORT_LOGGING_LEVEL_WARNING, "DataSentinel"),
      session_(nullptr),
      memory_info_(Ort::MemoryInfo::CreateCpu(OrtArenaAllocator, OrtMemTypeDefault)),
      expected_input_size_(0)
{
    const fs::path absolute_path = fs::absolute(model_path);
    if (!fs::exists(absolute_path))
    {
        throw std::runtime_error("Model file not found: " + absolute_path.string());
    }

    Ort::SessionOptions options;
    options.SetIntraOpNumThreads(1);
    options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_BASIC);

    session_ = Ort::Session(env_, absolute_path.c_str(), options);

    input_names_owned_ = session_.GetInputNames();
    output_names_owned_ = session_.GetOutputNames();

    for (const auto &name : input_names_owned_)
    {
        input_names_.push_back(name.c_str());
    }

    for (const auto &name : output_names_owned_)
    {
        output_names_.push_back(name.c_str());
    }

    expected_input_size_ = resolve_expected_input_size();
}

std::string OnnxInferenceBackend::backend_name() const
{
    return "onnx";
}

std::size_t OnnxInferenceBackend::expected_input_size() const
{
    return expected_input_size_;
}

std::vector<float> OnnxInferenceBackend::reconstruct(const std::vector<float> &input)
{
    if (input.size() != expected_input_size_)
    {
        throw std::runtime_error("Invalid input size for ONNX backend");
    }

    const std::vector<int64_t> input_shape = {1, static_cast<int64_t>(expected_input_size_)};

    Ort::Value input_tensor = Ort::Value::CreateTensor<float>(
        memory_info_,
        const_cast<float *>(input.data()),
        input.size(),
        input_shape.data(),
        input_shape.size());

    auto output_tensors = session_.Run(
        Ort::RunOptions{nullptr},
        input_names_.data(),
        &input_tensor,
        1,
        output_names_.data(),
        1);

    if (output_tensors.empty() || !output_tensors[0].IsTensor())
    {
        throw std::runtime_error("ONNX backend returned an invalid output tensor");
    }

    const auto output_info = output_tensors[0].GetTensorTypeAndShapeInfo();
    const std::size_t element_count = output_info.GetElementCount();
    if (element_count < expected_input_size_)
    {
        throw std::runtime_error("ONNX output tensor has fewer elements than expected");
    }

    const float *output_data = output_tensors[0].GetTensorData<float>();
    return std::vector<float>(output_data, output_data + expected_input_size_);
}

std::size_t OnnxInferenceBackend::resolve_expected_input_size() const
{
    const auto type_info = session_.GetInputTypeInfo(0);
    if (type_info.GetONNXType() != ONNX_TYPE_TENSOR)
    {
        throw std::runtime_error("Model input is not a tensor");
    }

    const auto tensor_info = type_info.GetTensorTypeAndShapeInfo();
    const auto shape = tensor_info.GetShape();
    if (shape.empty())
    {
        throw std::runtime_error("Input tensor has empty shape");
    }

    const int64_t feature_dim = (shape.size() > 1) ? shape[1] : shape[0];
    if (feature_dim <= 0)
    {
        throw std::runtime_error("Input tensor has non-static or invalid feature dimension");
    }

    return static_cast<std::size_t>(feature_dim);
}
} // namespace ds
