#pragma once

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
};
} // namespace ds
