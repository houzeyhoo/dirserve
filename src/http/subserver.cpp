
#include "subserver.hpp"
#include "transaction.hpp"
#include <unistd.h>

fd create_socket(long port)
{
    int server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
    if (server_fd == -1)
        return -1;
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)))
        return -1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEPORT, &opt, sizeof(opt)))
        return -1;
    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);
    if (bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0)
        return -1;
    if (listen(server_fd, 64) == -1)
        return -1;
    return server_fd;
}

namespace http
{

void subserver::run()
{
    listen_fd = create_socket(options.port);
    if (listen_fd == -1)
    {
        logger->error("Error creating socket");
        return;
    }

    utils::fd_guard listen_fd_guard(listen_fd);
    epoll_fd = epoll_create1(0);
    if (epoll_fd == -1)
    {
        logger->error("Error creating epoll instance");
        return;
    }
    utils::fd_guard epoll_fd_guard(epoll_fd);

    if (!epoll_add(listen_fd, EPOLLIN | EPOLLET))
    {
        logger->error("Error adding listening socket to epoll instance");
        return;
    }

    logger->info("Worker ready to accept connections");
    server_loop();
    logger->info("Worker has been shut down");
    clients.clear();
}

void subserver::server_loop()
{
    while (true)
    {
        std::array<epoll_event, 4096> events{};
        int n = epoll_wait(epoll_fd, events.data(), static_cast<int>(events.size()), 200);
        if (stopped)
            break;

        if (should_check_timeouts())
            handle_timeouts();

        if (n < 0)
        {
            logger->error("epoll_wait error occurred (errno={})", errno);
            continue;
        }

        for (int i = 0; i < n; i++)
        {
            current_event = &events.at(i);

            if (current_event->data.fd == listen_fd)
            {
                if (current_event->events & EPOLLERR)
                {
                    logger->debug("Listening socket has EPOLLERR event!");
                    continue;
                }
                handle_accept();
            }
            else
            {
                if (clients.empty())
                    continue;
                auto& tx = clients[current_event->data.fd];

                if ((current_event->events & EPOLLERR) || (current_event->events & EPOLLHUP) ||
                    (current_event->events & EPOLLRDHUP))
                {
                    handle_disconnect();
                    continue;
                }

                if (tx.is_reading() && (current_event->events & EPOLLIN))
                    handle_incoming();

                if (tx.is_writing() && (current_event->events & EPOLLOUT))
                    handle_outgoing();

                if (tx.is_complete())
                {
                    auto code = tx.get_response_code();
                    if (code != status_code::placeholder)
                    {
                        logger->request("{} -- {}", tx.get_status_line(), get_status_code_reason(code));
                    }
                    delete_client(current_event->data.fd);
                }
            }
        }
    }
}

void subserver::handle_accept()
{
    while (true)
    {
        if (clients.size() >= options.max_clients)
        {
            epoll_mod(listen_fd, EPOLLIN | EPOLLET);
            return;
        }

        fd client_fd = accept4(listen_fd, nullptr, nullptr, SOCK_NONBLOCK);
        if (client_fd == -1)
        {
            if (io_would_block())
                break;
            if (errno == EMFILE)
            {
                epoll_mod(listen_fd, EPOLLIN | EPOLLET);
                return;
            }
            logger->error("Error accepting connection (errno={})", errno);
            continue;
        }
        if (!add_client(client_fd))
            ::close(client_fd);
    }
}

void subserver::handle_incoming()
{
    fd client_fd = current_event->data.fd;
    auto& tx = clients[client_fd];
    while (tx.is_reading())
    {
        ssize_t read_count = tx.consume_data();
        if (read_count == -1)
        {
            if (!io_would_block())
            {
                logger->debug("Read error on client {}", client_fd);
                tx.mark_completed();
            }
            break;
        }
        if (read_count == 0)
        {
            tx.mark_completed();
            break;
        }
    }

    if (tx.is_writing())
    {
        if (!epoll_mod(client_fd, EPOLLOUT | EPOLLRDHUP | EPOLLET))
            tx.mark_completed();
    }
}

void subserver::handle_outgoing()
{
    fd client_fd = current_event->data.fd;
    auto& tx = clients[client_fd];
    while (tx.has_pending_data())
    {
        ssize_t write_count = tx.produce_data();
        if (write_count == -1)
        {
            if (io_would_block())
                break;
            logger->debug("Write error on client {}", client_fd);
            tx.mark_completed();
            return;
        }
    }

    if (!tx.has_pending_data() && tx.has_file())
    {
        while (tx.has_pending_file_data())
        {
            ssize_t write_count = tx.produce_file_data();
            if (write_count == -1)
            {
                if (io_would_block())
                    break;
                logger->debug("Write error sending file to client {}", client_fd);
                tx.mark_completed();
                return;
            }
            if (write_count == 0)
            {
                tx.mark_completed();
                return;
            }
        }
    }

    if (!tx.has_pending_data() && !tx.has_pending_file_data())
    {
        tx.mark_completed();
    }
}

void subserver::handle_disconnect()
{
    delete_client(current_event->data.fd);
}

void subserver::handle_timeouts()
{
    using namespace std::chrono;
    auto now = steady_clock::now();
    size_t old_size = clients.size();
    for (auto it = clients.begin(); it != clients.end();)
    {
        auto& [client_fd, tx] = *it;
        if (now - tx.get_creation_time() > seconds(30))
        {
            it = delete_client(client_fd);
        }
        else
        {
            ++it;
        }
    }
    size_t new_size = clients.size();
    if (old_size > new_size)
        logger->debug("Removed {} stale connections", old_size - new_size);
    last_timeout_check = now;
}

bool subserver::add_client(fd client_fd)
{
    if (!epoll_add(client_fd, EPOLLIN | EPOLLRDHUP | EPOLLET))
        return false;
    if (clients.contains(client_fd))
    {
        logger->warning("Attempted to overwrite existing client connection!");
        return false;
    }
    clients.emplace(client_fd, transaction(client_fd, options.directory_fd));
    logger->debug("Connected to client {}", client_fd);
    return true;
}

subserver::transaction_map::iterator subserver::delete_client(fd client_fd)
{
    auto it = clients.find(client_fd);
    if (it == clients.end())
    {
        logger->warning("Attempted to remove non-existent client!");
        return clients.end();
    }
    logger->debug("Disconnected from client {}", client_fd);
    return clients.erase(it);
}

bool subserver::epoll_add(fd target_fd, unsigned int events)
{
    epoll_event ev{};
    ev.data.fd = target_fd;
    ev.events = events;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, target_fd, &ev) == -1)
    {
        logger->error("Failed to add descriptor to epoll (errno={})", errno);
        return false;
    }
    return true;
}

bool subserver::epoll_mod(fd target_fd, unsigned int events)
{
    epoll_event ev{};
    ev.data.fd = target_fd;
    ev.events = events;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, target_fd, &ev) == -1)
    {
        logger->error("Failed to modify descriptor in epoll (errno={})", errno);
        return false;
    }
    return true;
}

} // namespace http