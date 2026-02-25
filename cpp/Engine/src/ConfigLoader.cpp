#include "ConfigLoader.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>

#include "json.hpp"

namespace fs = std::filesystem;
using json = nlohmann::json;

namespace ds
{
double load_threshold(const std::string &config_path)
{
    const fs::path path = fs::absolute(config_path);

    if (!fs::exists(path))
    {
        throw std::runtime_error("Config file not found: " + path.string());
    }

    std::ifstream config_stream(path);
    if (!config_stream.is_open())
    {
        throw std::runtime_error("Failed to open config file: " + path.string());
    }

    json payload;
    config_stream >> payload;

    if (!payload.contains("threshold") || !payload["threshold"].is_number())
    {
        throw std::runtime_error("Invalid or missing 'threshold' in config");
    }

    return payload["threshold"].get<double>();
}
} // namespace ds
