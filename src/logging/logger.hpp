
#pragma once
#include "severity.hpp"
#include "threading/pool.hpp"
#include "writer.hpp"
#include <memory>

namespace logging
{

class logger
{
  public:
    explicit logger(std::unique_ptr<writer> writer) : internal_writer(std::move(writer)) {};

    virtual ~logger() = default;

    logger(const logger&) = delete;
    logger& operator=(const logger&) = delete;
    logger(logger&&) noexcept = default;
    logger& operator=(logger&&) noexcept = default;

    template <typename... _fmt_args>
    void debug([[maybe_unused]] const std::string& message, [[maybe_unused]] _fmt_args&&... fmt_args);

    template <typename... _fmt_args> void request(const std::string& message, _fmt_args&&... fmt_args);

    template <typename... _fmt_args> void info(const std::string& message, _fmt_args&&... fmt_args);

    template <typename... _fmt_args> void warning(const std::string& message, _fmt_args&&... fmt_args);

    template <typename... _fmt_args> void error(const std::string& message, _fmt_args&&... fmt_args);

  protected:
    virtual void log(severity level, std::string message) = 0;

    const writer* get_writer() const
    {
        return internal_writer.get();
    };

  private:
    std::unique_ptr<writer> internal_writer;
};

class sync_logger : public logger
{
  public:
    using logger::logger;

  private:
    void log(severity level, std::string message) override;
};

class async_logger : public logger
{
  public:
    explicit async_logger(std::unique_ptr<writer> writer, size_t pool_size = 1)
        : logger(std::move(writer)), pool(pool_size, threading::pool::close_policy::discard_tasks) {};

  private:
    threading::fifo_pool pool;

    void log(severity level, std::string message) override;
};

template <typename... _fmt_args>
void logger::debug([[maybe_unused]] const std::string& message, [[maybe_unused]] _fmt_args&&... fmt_args)
{
#ifndef NDEBUG
    auto msg = fmt::format(fmt::runtime(message), fmt_args...);
    log(severity::Debug, std::move(msg));
#endif
}

template <typename... _fmt_args> void logger::request(const std::string& message, _fmt_args&&... fmt_args)
{
    auto msg = fmt::format(fmt::runtime(message), std::forward<_fmt_args>(fmt_args)...);
    log(severity::Request, std::move(msg));
}

template <typename... _fmt_args> void logger::info(const std::string& message, _fmt_args&&... fmt_args)
{
    auto msg = fmt::format(fmt::runtime(message), std::forward<_fmt_args>(fmt_args)...);
    log(severity::Info, std::move(msg));
}

template <typename... _fmt_args> void logger::warning(const std::string& message, _fmt_args&&... fmt_args)
{
    auto msg = fmt::format(fmt::runtime(message), std::forward<_fmt_args>(fmt_args)...);
    log(severity::Warning, std::move(msg));
}

template <typename... _fmt_args> void logger::error(const std::string& message, _fmt_args&&... fmt_args)
{
    auto msg = fmt::format(fmt::runtime(message), std::forward<_fmt_args>(fmt_args)...);
    log(severity::Error, std::move(msg));
}

} // namespace logging