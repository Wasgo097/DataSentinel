#pragma once

#include <onnxruntime_cxx_api.h>

#include <string>
#include <vector>

#include "IInferenceBackend.hpp"

namespace ds
{
class OnnxInferenceBackend : public IInferenceBackend
{
public:
    explicit OnnxInferenceBackend(const std::string &model_path);

    std::string backend_name() const override;
    std::size_t expected_input_size() const override;
    std::vector<float> reconstruct(const std::vector<float> &input) override;

private:
    std::size_t resolve_expected_input_size() const;

    Ort::Env env_;
    Ort::Session session_;
    Ort::MemoryInfo memory_info_;

    std::vector<std::string> input_names_owned_;
    std::vector<std::string> output_names_owned_;
    std::vector<const char *> input_names_;
    std::vector<const char *> output_names_;

    std::size_t expected_input_size_;
};
} // namespace ds
