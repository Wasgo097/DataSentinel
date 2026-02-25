#include "TensorRtInferenceBackend.hpp"

#include <filesystem>
#include <stdexcept>

#include <onnxruntime_cxx_api.h>

#include "Logger.hpp"
#include "TensorRtBuildOptions.hpp"
#include "TensorRtEngineStore.hpp"

namespace ds
{
namespace
{
std::size_t resolve_expected_input_size_from_onnx(const std::filesystem::path &onnx_path)
{
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "DataSentinelTensorRtInputShape");
    Ort::SessionOptions options;
    options.SetIntraOpNumThreads(1);
    options.SetGraphOptimizationLevel(GraphOptimizationLevel::ORT_ENABLE_BASIC);

    Ort::Session session(env, onnx_path.c_str(), options);

    const auto input_type_info = session.GetInputTypeInfo(0);
    if (input_type_info.GetONNXType() != ONNX_TYPE_TENSOR)
    {
        throw std::runtime_error("TensorRT backend expects tensor input, but ONNX input type is different");
    }

    const auto tensor_info = input_type_info.GetTensorTypeAndShapeInfo();
    const auto shape = tensor_info.GetShape();
    if (shape.empty())
    {
        throw std::runtime_error("ONNX input tensor has empty shape");
    }

    const int64_t feature_dim = (shape.size() > 1) ? shape[1] : shape[0];
    if (feature_dim <= 0)
    {
        throw std::runtime_error("ONNX input tensor has non-static or invalid feature dimension");
    }

    return static_cast<std::size_t>(feature_dim);
}
} // namespace

TensorRtInferenceBackend::TensorRtInferenceBackend(const std::string &model_path)
    : model_path_(std::filesystem::absolute(model_path))
{
#if DS_ENABLE_TENSORRT
    if (!std::filesystem::exists(model_path_))
    {
        throw std::runtime_error("Model file not found: " + model_path_.string());
    }

    TensorRtBuildOptions build_options{};
    TensorRtEngineBuilder builder(build_options);
    TensorRtEngineStore store(std::move(builder));
    engine_path_ = store.ensure_engine_file(model_path_);

    expected_input_size_ = resolve_expected_input_size_from_onnx(model_path_);

    ds::log::info("TensorRT engine file: " + engine_path_.string());
#else
    (void)model_path_;
    throw std::runtime_error("TensorRT backend is unavailable because binary was built without TensorRT support");
#endif
}

std::string TensorRtInferenceBackend::backend_name() const
{
    return "tensorrt";
}

std::size_t TensorRtInferenceBackend::expected_input_size() const
{
    return expected_input_size_;
}

std::vector<float> TensorRtInferenceBackend::reconstruct(const std::vector<float> &input)
{
    std::ignore = input;
    throw std::runtime_error(
        "TensorRT inference path is not implemented yet. Engine file was prepared at: " + engine_path_.string());
}
} // namespace ds
