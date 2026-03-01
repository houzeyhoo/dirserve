
#include "writer.hpp"
#include <fmt/chrono.h>
#include <syncstream>

namespace logging
{

stream_writer::~stream_writer()
{
    if (owns_stream)
        delete output_stream;
}

void stream_writer::write(severity level, std::string message, size_t thread_id) const
{
    if (output_stream == nullptr)
        return;

    using namespace std::chrono;
    auto const now = time_point_cast<milliseconds>(system_clock::now());

    auto sync_stream = std::osyncstream(*output_stream);

    using namespace fmt;
    auto level_string = format("{:<8}", get_severity_name(level));
    auto timestamp_string = format("{}", now);
    auto thread_string = format("{:<5}", thread_id);

    if (decorated)
    {
        fmt::println(sync_stream, "{} {} {} {}", styled(timestamp_string, fg(color::dim_gray)),
                     styled(thread_string, emphasis::italic | fg(color::dim_gray)),
                     styled(level_string, emphasis::bold | fg(get_severity_color(level))), message);
    }
    else
    {
        fmt::println(sync_stream, "{} {} {} {}", timestamp_string, thread_string, level_string, message);
    }
}

} // namespace logging
