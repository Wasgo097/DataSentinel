#pragma once

#include <memory>
#include <string>

#include "IInferenceBackend.hpp"

namespace ds
{
enum class BackendKind
{
    Onnx,
    TensorRt
};

BackendKind parse_backend_kind(const std::string &backend_name);
std::unique_ptr<IInferenceBackend> create_backend(BackendKind kind,
                                                  const std::string &model_path);
} // namespace ds
