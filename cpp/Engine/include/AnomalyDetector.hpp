#pragma once

#include <string>
#include <vector>

#include "IInferenceBackend.hpp"

namespace ds
{
enum class DetectionStatus
{
    Ok,
    Anomaly
};

struct DetectionResult
{
    double mse;
    DetectionStatus status;

    std::string response_line() const;
};

class AnomalyDetector
{
public:
    AnomalyDetector(IInferenceBackend &backend, double threshold);

    DetectionResult evaluate(const std::vector<float> &input);

private:
    IInferenceBackend &backend_;
    double threshold_;
};
} // namespace ds
