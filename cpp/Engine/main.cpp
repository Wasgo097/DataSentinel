#include <cstdlib>
#include <exception>
#include <iostream>
#include <string>

#include "AnomalyDetector.hpp"
#include "Config.hpp"
#include "ConfigLoader.hpp"
#include "InferenceBackendFactory.hpp"
#include "Logger.hpp"
#include "TcpServer.hpp"

namespace ds
{
namespace
{
std::string resolve_backend_name()
{
    const char *env_backend = std::getenv("DATASENTINEL_BACKEND");
    if (env_backend == nullptr)
    {
        return "onnx";
    }

    return std::string(env_backend);
}
} // namespace
} // namespace ds

int main()
{
    try
    {
        const ds::AppConfig config{};
        const std::string backend_name = ds::resolve_backend_name();
        const ds::BackendKind backend_kind = ds::parse_backend_kind(backend_name);

        auto backend = ds::create_backend(backend_kind, config.model_path);
        const double threshold = ds::load_threshold(config.runtime_config_path);

        ds::log::info("Backend: " + backend->backend_name());
        ds::log::info("Threshold: " + std::to_string(threshold));
        ds::log::info("Expected input size: " + std::to_string(backend->expected_input_size()));

        ds::AnomalyDetector detector(*backend, threshold);
        ds::TcpServer server(config.server_port, detector, backend->expected_input_size());
        server.run();
    }
    catch (const std::exception &ex)
    {
        std::cerr << "Fatal error: " << ex.what() << '\n';
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
