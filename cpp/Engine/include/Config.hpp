#pragma once

#include <cstdint>
#include <string>

namespace ds
{
struct AppConfig
{
    std::string model_path{"models/model.onnx"};
    std::string runtime_config_path{"models/config.json"};
    std::uint16_t server_port{9000};
};
} // namespace ds
