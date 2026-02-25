#pragma once

#include <cstddef>
#include <string>
#include <vector>

namespace ds
{
class IInferenceBackend
{
public:
    virtual ~IInferenceBackend() = default;

    virtual std::string backend_name() const = 0;
    virtual std::size_t expected_input_size() const = 0;
    virtual std::vector<float> reconstruct(const std::vector<float> &input) = 0;
};
} // namespace ds
