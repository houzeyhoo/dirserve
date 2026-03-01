
#pragma once
#include <utility>

using fd = int;

namespace utils
{

class fd_guard
{
  public:
    fd_guard() = default;

    explicit fd_guard(fd existing) : fd_(existing) {};

    ~fd_guard()
    {
        close_if_needed();
    };

    fd_guard(const fd_guard&) = delete;
    fd_guard& operator=(const fd_guard&) = delete;

    fd_guard(fd_guard&& other) noexcept;

    fd_guard& operator=(fd_guard&& other) noexcept;

    fd get() const
    {
        return fd_;
    };

    bool valid() const
    {
        return fd_ != -1;
    }

    fd release()
    {
        return std::exchange(fd_, -1);
    };

    void replace(fd existing);

  private:
    fd fd_ = -1;

    void close_if_needed();
};

} // namespace utils