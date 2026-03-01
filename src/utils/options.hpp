
#pragma once
#include <filesystem>
#include <fstream>
#include <unistd.h>

namespace utils
{

struct options
{
    long server_count = 0;

    long port = 0;

    long max_clients = 65535;

    std::string log_output;

    std::string server_dir = "/var/www/dirserve/";

    bool show_help = false;
};

inline std::string get_help_string()
{
    return "PLACEHOLDER";
}

inline options parse_arguments(int argc, char** argv)
{
    options opts{};
    int opt = 0;
    while ((opt = getopt(argc, argv, "s:p:c:o:d:h")) != -1)
    {
        switch (opt)
        {
        case 's':
            opts.server_count = std::strtol(optarg, nullptr, 10);
            if (opts.server_count <= 0)
                throw std::invalid_argument("Invalid argument: number of workers (-s)");
            break;
        case 'p':
            opts.port = std::strtol(optarg, nullptr, 10);
            if (opts.port <= 0 || opts.port > 65535)
                throw std::invalid_argument("Invalid argument: port (-p)");
            break;
        case 'c':
            opts.max_clients = std::strtol(optarg, nullptr, 10);
            if (opts.max_clients <= 0 || opts.max_clients > 65535)
            {
                throw std::invalid_argument("Invalid argument: maximum number of clients (-c)");
            }
            break;
        case 'o':
            opts.log_output = optarg;
            {
                std::ofstream tmp(opts.log_output);
                if (tmp.fail())
                {
                    throw std::invalid_argument("Invalid argument: log file path (-o)");
                }
            }
            break;
        case 'd':
            opts.server_dir = optarg;
            if (!std::filesystem::is_directory(opts.server_dir))
            {
                throw std::invalid_argument("Invalid argument: server directory path (-d)");
            }
            break;
        case 'h':
            opts.show_help = true;
            return opts;
        default:
            break;
        }
    }

    if (opts.server_count == 0)
        throw std::invalid_argument("Missing required argument: number of servers (-s)");
    if (opts.port == 0)
        throw std::invalid_argument("Missing required argument: port (-p)");

    return opts;
}

} // namespace utils