
#pragma once
#include "http/subserver.hpp"
#include "logging/logger.hpp"
#include "threading/pool.hpp"
#include "utils/fd_guard.hpp"
#include <memory>
#include <utility>

namespace http
{

struct server_options
{
    long subserver_count;

    long port;

    unsigned long max_clients;

    std::string server_dir;
};

class server
{
  public:
    server(std::shared_ptr<logging::logger> logger, server_options options)
        : logger(std::move(logger)), pool(options.subserver_count), options(std::move(options))
    {
    }

    ~server() = default;

    server(const server&) = delete;
    server& operator=(const server&) = delete;
    server(server&& other) noexcept = delete;
    server& operator=(server&& other) noexcept = delete;

    void run();

    void stop();

  private:
    std::vector<std::unique_ptr<subserver>> subservers;

    std::vector<std::future<void>> run_futures;

    std::shared_ptr<logging::logger> logger;

    utils::fd_guard directory_fd_guard;

    threading::fifo_pool pool;

    server_options options;
};

} // namespace http