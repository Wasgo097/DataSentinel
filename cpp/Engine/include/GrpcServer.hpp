#pragma once

#include <cstddef>
#include <cstdint>

#include "AnomalyDetector.hpp"

namespace ds
{
class GrpcServer
{
public:
    GrpcServer(std::uint16_t port,
               AnomalyDetector &detector,
               std::size_t expected_input_size);

    void run();

private:
    std::uint16_t port_;
    AnomalyDetector &detector_;
    std::size_t expected_input_size_;
};
} // namespace ds
