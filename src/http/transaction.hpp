
#pragma once
#include "helpers.hpp"
#include "utils/fd_guard.hpp"
#include <chrono>

namespace http
{

enum class transaction_state
{
    reading,
    writing,
    completed,
};

class transaction
{
  public:
    transaction() = default;

    transaction(fd client_fd, fd server_directory_fd);

    ~transaction() = default;

    transaction(const transaction&) = delete;
    transaction& operator=(const transaction&) = delete;
    transaction(transaction&&) noexcept = default;
    transaction& operator=(transaction&&) noexcept = default;

    ssize_t consume_data();

    ssize_t produce_data();
    ssize_t produce_file_data();

    bool has_file() const
    {
        return file_guard.valid();
    }

    bool has_pending_data() const
    {
        return data_offset < data.size();
    }

    bool has_pending_file_data() const
    {
        return file_offset < file_size;
    }

    bool is_reading() const
    {
        return state == transaction_state::reading;
    }
    bool is_writing() const
    {
        return state == transaction_state::writing;
    }
    bool is_complete() const
    {
        return state == transaction_state::completed;
    }

    void mark_completed()
    {
        state = transaction_state::completed;
    };

    status_code get_response_code() const
    {
        return response_code;
    }

    std::string get_status_line() const
    {
        return method + " " + uri + " " + version;
    }

    std::chrono::steady_clock::time_point get_creation_time() const
    {
        return creation_time;
    }

  private:
    utils::fd_guard client_guard;

    transaction_state state = transaction_state::reading;

    std::chrono::steady_clock::time_point creation_time;

    std::string method, uri, version;

    status_code response_code = status_code::placeholder;

    std::string data = "";
    size_t data_offset = 0;

    utils::fd_guard file_guard;
    fd server_directory_fd = -1;
    off_t file_offset = 0;
    long file_size = 0;

    void process_data(const std::string& new_data);

    void prepare_response(status_code code);

    void prepare_response_from_data();

    void prepare_file(bool send_content);
};

} // namespace http