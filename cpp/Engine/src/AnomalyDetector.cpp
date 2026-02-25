#include "AnomalyDetector.hpp"

#include <stdexcept>

namespace ds
{
namespace
{
double compute_mse(const std::vector<float> &actual,
                   const std::vector<float> &reconstructed)
{
    if (actual.size() != reconstructed.size())
    {
        throw std::runtime_error("MSE requires vectors of equal length");
    }

    double mse = 0.0;
    for (std::size_t i = 0; i < actual.size(); ++i)
    {
        const double diff = static_cast<double>(reconstructed[i]) - static_cast<double>(actual[i]);
        mse += diff * diff;
    }

    return mse / static_cast<double>(actual.size());
}
} // namespace

std::string DetectionResult::response_line() const
{
    return (status == DetectionStatus::Anomaly) ? "ANOMALY\n" : "OK\n";
}

AnomalyDetector::AnomalyDetector(IInferenceBackend &backend, double threshold)
    : backend_(backend),
      threshold_(threshold)
{
}

DetectionResult AnomalyDetector::evaluate(const std::vector<float> &input)
{
    const auto reconstructed = backend_.reconstruct(input);
    const double mse = compute_mse(input, reconstructed);

    return DetectionResult{
        .mse = mse,
        .status = (mse > threshold_) ? DetectionStatus::Anomaly : DetectionStatus::Ok,
    };
}
} // namespace ds
