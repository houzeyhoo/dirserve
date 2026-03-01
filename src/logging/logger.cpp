
#include "logger.hpp"

namespace logging
{

void sync_logger::log(severity level, std::string message)
{
    int thread_id = gettid();
    get_writer()->write(level, std::move(message), thread_id);
}

void async_logger::log(severity level, std::string message)
{
    int thread_id = gettid();
    pool.enqueue(
        [this, level, message = std::move(message), thread_id] { get_writer()->write(level, message, thread_id); });
}

} // namespace logging