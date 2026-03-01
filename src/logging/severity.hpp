
#pragma once
#include <fmt/color.h>
#include <map>

namespace logging
{

enum class severity
{
    Debug = 0,
    Request,
    Info,
    Warning,
    Error,
};

struct _severity_attributes
{
    std::string name;
    fmt::color color;
};

inline const std::map<severity, _severity_attributes> _severity_attribute_dict{
    {severity::Debug, {"DEBUG", fmt::color::dodger_blue}}, {severity::Request, {"REQUEST", fmt::color::floral_white}},
    {severity::Info, {"INFO", fmt::color::forest_green}},  {severity::Warning, {"WARNING", fmt::color::yellow}},
    {severity::Error, {"ERROR", fmt::color::red}},
};

std::string_view get_severity_name(severity level);

fmt::color get_severity_color(severity level);

} // namespace logging