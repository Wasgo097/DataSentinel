#pragma once

#include <cstddef>

namespace ds
{
struct TensorRtBuildOptions
{
    constexpr static std::size_t max_workspace_size_bytes{1024*1024*1024}; // 1GB
    constexpr static bool enable_fp16{true};
};
} // namespace ds
