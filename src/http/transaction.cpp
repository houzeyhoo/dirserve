
#include "transaction.hpp"
#include "helpers.hpp"
#include <fcntl.h>
#include <array>
#include <filesystem>
#include <linux/openat2.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <unistd.h>

namespace http
{

transaction::transaction(fd client_fd, fd server_directory_fd)
    : client_guard(client_fd), creation_time(std::chrono::steady_clock::now()), server_directory_fd(server_directory_fd)
{
}

ssize_t transaction::consume_data()
{
    std::array<char, 2048> buffer{};
    ssize_t read_count = read(client_guard.get(), buffer.data(), buffer.size());
    if (read_count > 0)
    {
        process_data(std::string(buffer.data(), static_cast<size_t>(read_count)));
    }
    return read_count;
}

ssize_t transaction::produce_data()
{
    const char* data_pending = data.data() + data_offset;
    size_t pending_data_size = data.size() - data_offset;
    ssize_t write_count = write(client_guard.get(), data_pending, pending_data_size);
    if (write_count > 0)
        data_offset += write_count;
    return write_count;
}

ssize_t transaction::produce_file_data()
{
    ssize_t write_bytes = sendfile(client_guard.get(), file_guard.get(), &file_offset, file_size - file_offset);
    return write_bytes;
}

void transaction::process_data(const std::string& new_data)
{
    data += new_data;
    if (data.size() > 16 * 1024)
    {
        prepare_response(status_code::uri_too_long);
        return;
    }
    if (data.find("\r\n") != std::string::npos)
        prepare_response_from_data();
}

void transaction::prepare_response(status_code code)
{
    data = make_error_response(code);
    data_offset = 0;
    state = transaction_state::writing;
    response_code = code;
}

void transaction::prepare_response_from_data()
{
    std::istringstream iss(data);
    if (!(iss >> method >> uri >> version))
    {
        prepare_response(status_code::bad_request);
        return;
    }
    if (!version.starts_with("HTTP/1"))
    {
        prepare_response(status_code::http_version_not_supported);
        return;
    }
    if (method == "GET" || method == "HEAD")
        prepare_file(method == "GET");
    else
        prepare_response(status_code::method_not_allowed);
}

void transaction::prepare_file(bool send_content)
{
    std::string path = uri;

    while (!path.empty() && path.front() == '/')
        path.erase(0, 1);

    auto qmark_pos = path.find('?');
    if (qmark_pos != std::string::npos)
        path = path.substr(0, qmark_pos);

    if (path.empty())
        path = "index.html";
    auto extension = std::filesystem::path(path).extension();
    if (extension.empty())
    {
        extension = ".html";
        path += extension;
    }

    open_how options = {.flags = O_RDONLY | O_NONBLOCK, .mode = 0, .resolve = RESOLVE_IN_ROOT};
    utils::fd_guard tmp(syscall(SYS_openat2, server_directory_fd, path.c_str(), &options, sizeof(options)));
    if (!tmp.valid())
    {
        if (errno == EMFILE)
        {
            prepare_response(status_code::service_unavailable);
            return;
        }
        prepare_response(status_code::not_found);
        return;
    }

    struct stat file_stats = {};
    if (fstat(tmp.get(), &file_stats) == -1)
    {
        prepare_response(status_code::internal_server_error);
        return;
    }

    response_code = status_code::ok;
    state = transaction_state::writing;
    header_dict headers{{"Content-Length", std::to_string(file_stats.st_size)},
                        {"Content-Type", get_mime_type(extension)}};
    data = make_header_section(status_code::ok, headers);
    data_offset = 0;
    if (send_content)
    {
        file_guard = std::move(tmp);
        file_size = file_stats.st_size;
        file_offset = 0;
    }
}

} // namespace http
