#include "TensorRtEngineStore.hpp"

#include <filesystem>
#include <fstream>
#include <stdexcept>

#include "Logger.hpp"
#include "TensorRtEnginePathResolver.hpp"

namespace ds
{
TensorRtEngineStore::TensorRtEngineStore(TensorRtEngineBuilder builder)
    : builder_(std::move(builder))
{
}

std::filesystem::path TensorRtEngineStore::ensure_engine_file(const std::filesystem::path &onnx_model_path) const
{
    const auto onnx_absolute = std::filesystem::absolute(onnx_model_path);
    const auto engine_path = TensorRtEnginePathResolver::resolve_engine_path(onnx_absolute);

    if (std::filesystem::exists(engine_path))
    {
        ds::log::info("TensorRT engine imported from: " + engine_path.string());
        return engine_path;
    }

    std::filesystem::create_directories(engine_path.parent_path());
    const auto serialized = builder_.build_serialized_engine(onnx_absolute);

    std::ofstream output(engine_path, std::ios::binary | std::ios::trunc);
    if (!output.is_open())
    {
        throw std::runtime_error("Failed to open TensorRT engine output file: " + engine_path.string());
    }

    output.write(serialized.data(), static_cast<std::streamsize>(serialized.size()));
    if (!output.good())
    {
        throw std::runtime_error("Failed to write TensorRT engine file: " + engine_path.string());
    }

    ds::log::info("TensorRT engine exported to: " + engine_path.string());
    return engine_path;
}
} // namespace ds
