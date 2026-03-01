
#include "server.hpp"
#include "subserver.hpp"
#include <fcntl.h>

namespace http
{

void server::run()
{
    logger->info("Starting server on port {} with root directory \"{}\"", options.port, options.server_dir);

    directory_fd_guard.replace(open(options.server_dir.c_str(), O_DIRECTORY | O_RDONLY));
    if (!directory_fd_guard.valid())
    {
        logger->error("Failed to open server directory (errno={})", errno);
        return;
    }

    subserver_options subserver_config{
        .port = options.port,
        .max_clients = options.max_clients,
        .directory_fd = directory_fd_guard.get(),
    };

    subservers.reserve(options.subserver_count);
    for (int i = 0; i < options.subserver_count; i++)
    {
        subservers.push_back(std::make_unique<http::subserver>(logger, subserver_config));
        run_futures.push_back(pool.enqueue([instance = &subservers[i]] { instance->get()->run(); }));
    }
}

void server::stop()
{
    logger->info("Server is shutting down");
    for (auto& s : subservers)
        s->stop();
    for (auto& f : run_futures)
        f.wait();
    logger->info("Server shut down");
}

} // namespace http