
#include "helpers.hpp"
#include <sstream>

namespace http
{

std::string get_mime_type(const std::string& extension)
{
    static const std::unordered_map<std::string, std::string> mime_type_dict{
        {".html", "text/html"},
        {".htm", "text/html"},
        {".css", "text/css"},
        {".txt", "text/plain"},
        {".png", "image/png"},
        {".jpg", "image/jpeg"},
        {".jpeg", "image/jpeg"},
        {".gif", "image/gif"},
        {".svg", "image/svg+xml"},
        {".json", "application/json"},
        {".js", "application/javascript"},
        {".pdf", "application/pdf"},
        {".xml", "application/xml"},
    };
    auto it = mime_type_dict.find(extension);
    if (it != mime_type_dict.end())
        return it->second;
    return "text/plain";
}

std::string get_status_code_reason(status_code code)
{
    static const std::unordered_map<status_code, std::string> status_code_reason_dict{
        {status_code::ok, "200 OK"},
        {status_code::bad_request, "400 Bad Request"},
        {status_code::not_found, "404 Not Found"},
        {status_code::method_not_allowed, "405 Method Not Allowed"},
        {status_code::uri_too_long, "414 URI Too Long"},
        {status_code::internal_server_error, "500 Internal Server Error"},
        {status_code::service_unavailable, "503 Service Unavailable"},
        {status_code::http_version_not_supported, "505 HTTP Version Not Supported"},

    };
    auto it = status_code_reason_dict.find(code);
    if (it != status_code_reason_dict.end())
        return it->second;
    return "";
}

std::string make_header_section(status_code code, const header_dict& headers)
{
    auto reason = get_status_code_reason(code);
    std::ostringstream oss;
    oss << "HTTP/1.0" << " " << reason << "\r\n";
    for (const auto& [k, v] : headers)
        oss << k << ": " << v << "\r\n";
    oss << "\r\n";
    return oss.str();
}

std::string make_error_response(status_code code)
{
    auto reason = get_status_code_reason(code);
    auto body = "<h1>" + reason + "</h1>";
    header_dict headers{
        {"Content-Length", std::to_string(body.size())},
        {"Content-Type", "text/html"},
    };
    return make_header_section(code, headers) + body;
}

} // namespace http
