
#include "severity.hpp"

namespace logging
{

std::string_view get_severity_name(severity level)
{
    if (_severity_attribute_dict.contains(level))
        return _severity_attribute_dict.at(level).name;
    return "UNKNOWN";
}

fmt::color get_severity_color(severity level)
{
    if (_severity_attribute_dict.contains(level))
        return _severity_attribute_dict.at(level).color;
    return fmt::color::gray;
}

}; // namespace logging