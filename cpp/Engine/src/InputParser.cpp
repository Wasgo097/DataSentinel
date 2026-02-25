#include "InputParser.hpp"

#include <sstream>

namespace ds
{
std::vector<float> parse_input_line(const std::string &input)
{
    std::vector<float> values;
    std::stringstream stream(input);
    float value = 0.0F;

    while (stream >> value)
    {
        values.push_back(value);
    }

    return values;
}
} // namespace ds
