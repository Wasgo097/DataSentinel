#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

#include "IInferenceBackend.hpp"

namespace ds
{
class TensorRtInferenceBackend : public IInferenceBackend
{
public:
    explicit TensorRtInferenceBackend(const std::string &model_path);
    ~TensorRtInferenceBackend() override;

    TensorRtInferenceBackend(const TensorRtInferenceBackend &) = delete;
    TensorRtInferenceBackend &operator=(const TensorRtInferenceBackend &) = delete;
    TensorRtInferenceBackend(TensorRtInferenceBackend &&) noexcept;
    TensorRtInferenceBackend &operator=(TensorRtInferenceBackend &&) noexcept;

    std::string backend_name() const override;
    std::size_t expected_input_size() const override;
    std::vector<float> reconstruct(const std::vector<float> &input) override;

private:
    std::filesystem::path model_path_;
    std::filesystem::path engine_path_;
    std::size_t expected_input_size_{0};
    class Impl;
    std::unique_ptr<Impl> impl_;
};
} // namespace ds
