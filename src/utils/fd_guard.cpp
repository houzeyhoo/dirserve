
#include "fd_guard.hpp"
#include <unistd.h>

namespace utils
{

fd_guard::fd_guard(fd_guard&& other) noexcept : fd_(other.fd_)
{
    other.fd_ = -1;
};

fd_guard& fd_guard::operator=(fd_guard&& other) noexcept
{
    if (this != &other)
    {
        close_if_needed();
        fd_ = std::exchange(other.fd_, -1);
    }
    return *this;
}

void fd_guard::replace(fd existing)
{
    close_if_needed();
    fd_ = existing;
}

void fd_guard::close_if_needed()
{
    if (fd_ != -1)
        close(fd_);
}

} // namespace utils
