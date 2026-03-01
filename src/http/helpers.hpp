
#pragma once
#include <string>
#include <unordered_map>

namespace http
{

using header_dict = std::unordered_map<std::string, std::string>;

enum class status_code
{
    placeholder = 0,
    ok = 200,
    bad_request = 400,
    not_found = 404,
    method_not_allowed = 405,
    uri_too_long = 414,
    internal_server_error = 500,
    service_unavailable = 503,
    http_version_not_supported = 505,
};

std::string get_mime_type(const std::string& extension);

std::string get_status_code_reason(status_code code);

std::string make_header_section(status_code code, const header_dict& headers);

std::string make_error_response(status_code code);

} // namespace http