#include "InferenceBackendFactory.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

#include "OnnxInferenceBackend.hpp"
#include "TensorRtInferenceBackend.hpp"

namespace ds
{
namespace
{
std::string normalize_backend_name(const std::string &backend_name)
{
    std::string value = backend_name;
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
        return static_cast<char>(std::tolower(c));
    });
    return value;
}
} // namespace

BackendKind parse_backend_kind(const std::string &backend_name)
{
    const std::string value = normalize_backend_name(backend_name);

    if (value == "onnx")
    {
        return BackendKind::Onnx;
    }

    if (value == "tensorrt" || value == "trt" || value == "tensor")
    {
        return BackendKind::TensorRt;
    }

    throw std::runtime_error("Unsupported backend: " + backend_name + " (supported: onnx, tensorrt)");
}

std::unique_ptr<IInferenceBackend> create_backend(BackendKind kind,
                                                  const std::string &model_path)
{
    switch (kind)
    {
    case BackendKind::Onnx:
        return std::make_unique<OnnxInferenceBackend>(model_path);
    case BackendKind::TensorRt:
        return std::make_unique<TensorRtInferenceBackend>(model_path);
    }

    throw std::runtime_error("Unknown backend kind");
}
} // namespace ds
