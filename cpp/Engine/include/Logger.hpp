#pragma once

#include <iostream>
#include <string>

namespace ds::log
{
constexpr bool kDebugLog = true;

inline void info(const std::string &message)
{
    if (kDebugLog)
    {
        std::cout << "[INFO] " << message << std::endl;
    }
}

inline void error(const std::string &message)
{
    std::cerr << "[ERROR] " << message << std::endl;
}
} // namespace ds::log
