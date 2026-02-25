#pragma once

#include <filesystem>
#include <string>
#include <vector>

#include "IInferenceBackend.hpp"

namespace ds
{
class TensorRtInferenceBackend : public IInferenceBackend
{
public:
    explicit TensorRtInferenceBackend(const std::string &model_path);

    std::string backend_name() const override;
    std::size_t expected_input_size() const override;
    std::vector<float> reconstruct(const std::vector<float> &input) override;

private:
    std::filesystem::path model_path_;
    std::filesystem::path engine_path_;
    std::size_t expected_input_size_{0};
};
} // namespace ds
