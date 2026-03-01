
#pragma once
#include "logging/logger.hpp"
#include "transaction.hpp"
#include "utils/fd_guard.hpp"
#include <chrono>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <unordered_map>

namespace http
{

struct subserver_options
{
    long port;

    unsigned long max_clients;

    fd directory_fd;
};

class subserver
{
    using transaction_map = std::unordered_map<fd, transaction>;

  public:
    subserver(std::shared_ptr<logging::logger> logger, subserver_options options)
        : logger(std::move(logger)), options(options) {};

    ~subserver() = default;

    subserver(const subserver&) = delete;
    subserver& operator=(const subserver&) = delete;
    subserver(subserver&& other) noexcept = delete;
    subserver& operator=(subserver&& other) noexcept = delete;

    void run();

    void stop()
    {
        stopped = true;
    }

  private:
    std::shared_ptr<logging::logger> logger;

    std::atomic_bool stopped = false;

    transaction_map clients;

    subserver_options options;

    fd listen_fd = -1, epoll_fd = -1;

    std::chrono::steady_clock::time_point last_timeout_check;

    epoll_event* current_event = nullptr;

    void server_loop();

    void handle_accept();

    void handle_incoming();

    void handle_outgoing();

    void handle_disconnect();

    void handle_timeouts();

    bool add_client(fd client_fd);

    transaction_map::iterator delete_client(fd client_fd);

    bool epoll_add(fd target, unsigned int events);

    bool epoll_mod(fd target, unsigned int events);

    bool io_would_block() const
    {
        return errno == EAGAIN || errno == EWOULDBLOCK;
    }

    bool should_check_timeouts() const
    {
        return std::chrono::steady_clock::now() - last_timeout_check > std::chrono::seconds(30);
    }
};

} // namespace http