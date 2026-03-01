
#pragma once
#include "severity.hpp"
#include <fstream>

namespace logging
{

class writer
{
  public:
    writer() = default;

    virtual ~writer() = default;

    writer(const writer&) = delete;
    writer& operator=(const writer&) = delete;
    writer(writer&&) noexcept = default;
    writer& operator=(writer&&) noexcept = default;

    virtual void write(severity level, std::string message, size_t thread_id = 0) const = 0;
};

class stream_writer : public writer
{
  public:
    stream_writer(std::ostream* output_stream, bool decorated = false)
        : output_stream(output_stream), decorated(decorated)
    {
    }

    stream_writer(const std::string& output_path, bool decorated = false)
        : output_stream(new std::ofstream(output_path)), owns_stream(true), decorated(decorated)
    {
    }

    ~stream_writer();

    void write(severity level, std::string message, size_t thread_id = 0) const override;

  private:
    std::ostream* output_stream;

    bool owns_stream = false;

    bool decorated;
};

} // namespace logging