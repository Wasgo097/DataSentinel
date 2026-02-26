#include "GrpcServer.hpp"

#include <grpcpp/grpcpp.h>

#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "Logger.hpp"
#include "inference.grpc.pb.h"

namespace ds
{
namespace
{
class InferenceServiceImpl final : public datasentinel::v1::InferenceService::Service
{
public:
    InferenceServiceImpl(AnomalyDetector &detector, std::size_t expected_input_size)
        : detector_(detector),
          expected_input_size_(expected_input_size)
    {
    }

    grpc::Status Evaluate(grpc::ServerContext *context,
                          const datasentinel::v1::EvaluateRequest *request,
                          datasentinel::v1::EvaluateResponse *response) override
    {
        std::vector<float> values(request->values().begin(), request->values().end());
        std::ostringstream raw;
        for (std::size_t i = 0; i < values.size(); ++i)
        {
            if (i > 0)
            {
                raw << ' ';
            }
            raw << values[i];
        }

        ds::log::info("Client request: " + context->peer());
        ds::log::info("Received raw: " + raw.str());

        if (values.size() != expected_input_size_)
        {
            response->set_status(datasentinel::v1::EvaluateResponse::ERROR);
            response->set_message("Invalid input size. Expected " + std::to_string(expected_input_size_) +
                                  ", got " + std::to_string(values.size()));
            ds::log::error(response->message());
            ds::log::info("Sending response: ERROR");
            return grpc::Status::OK;
        }

        const auto result = detector_.evaluate(values);
        response->set_mse(result.mse);
        ds::log::info("Reconstruction MSE: " + std::to_string(result.mse));

        if (result.status == DetectionStatus::Anomaly)
        {
            response->set_status(datasentinel::v1::EvaluateResponse::ANOMALY);
            response->set_message("ANOMALY");
        }
        else
        {
            response->set_status(datasentinel::v1::EvaluateResponse::OK);
            response->set_message("OK");
        }

        ds::log::info("Sending response: " + response->message());

        return grpc::Status::OK;
    }

private:
    AnomalyDetector &detector_;
    std::size_t expected_input_size_;
};
} // namespace

GrpcServer::GrpcServer(std::uint16_t port,
                       AnomalyDetector &detector,
                       std::size_t expected_input_size)
    : port_(port),
      detector_(detector),
      expected_input_size_(expected_input_size)
{
}

void GrpcServer::run()
{
    InferenceServiceImpl service(detector_, expected_input_size_);
    grpc::ServerBuilder builder;

    const std::string address = "0.0.0.0:" + std::to_string(port_);
    builder.AddListeningPort(address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server(builder.BuildAndStart());
    if (server == nullptr)
    {
        throw std::runtime_error("Failed to start gRPC server on " + address);
    }

    ds::log::info("gRPC server listening on " + address);
    server->Wait();
}
} // namespace ds
