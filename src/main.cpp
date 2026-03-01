
#include "http/server.hpp"
#include "logging/logger.hpp"
#include "logging/writer.hpp"
#include "utils/options.hpp"
#include <bits/types/sigset_t.h>
#include <csignal>
#include <iostream>
#include <termios.h>

void setup_terminal()
{
    termios term{};
    if (tcgetattr(STDIN_FILENO, &term) == 0)
    {
        term.c_lflag &= ~ECHOCTL;
        tcsetattr(STDIN_FILENO, TCSANOW, &term);
    }
}

sigset_t setup_signals()
{
    std::signal(SIGPIPE, SIG_IGN);
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, SIGINT);
    pthread_sigmask(SIG_BLOCK, &set, nullptr);
    return set;
}

void wait_for_signal_set(sigset_t& set)
{
    int sig = 0;
    sigwait(&set, &sig);
}

int main(int argc, char** argv)
{
    setup_terminal();
    auto signal_set = setup_signals();

    utils::options app_options;
    try
    {
        app_options = utils::parse_arguments(argc, argv);
    }
    catch (const std::invalid_argument& e)
    {
        std::cout << e.what() << "\nHint: run the program with -h to see usage instructions.\n";
        return 1;
    }
    if (app_options.show_help)
    {
        std::cout << utils::get_help_string() << "\n";
        return 0;
    }

    std::unique_ptr<logging::stream_writer> writer;
    if (app_options.log_output == "")
    {
        writer = std::make_unique<logging::stream_writer>(&std::cout, true);
    }
    else
    {
        writer = std::make_unique<logging::stream_writer>(app_options.log_output);
    }

#ifndef NDEBUG
    auto logger = std::make_shared<logging::sync_logger>(std::move(writer));
#else
    auto logger = std::make_shared<logging::async_logger>(std::move(writer));
#endif

    http::server_options server_config{
        .subserver_count = app_options.server_count,
        .port = app_options.port,
        .max_clients = static_cast<unsigned long>(app_options.max_clients),
        .server_dir = app_options.server_dir,
    };

    http::server server(logger, server_config);
    server.run();

    wait_for_signal_set(signal_set);
    server.stop();
    return 0;
}
