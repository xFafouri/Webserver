#include "server.hpp"
#include <cstddef>
#include <sys/socket.h>



Server::Server(int domain, int type, int protocol)
{
    // domain = AF_INET;
    // type = SOCK_STREAM;
    // protocol = 0;

    server_fd = socket(domain, type, protocol);

    address.sin_family = AF_INET;
    address.sin_port = htons(8080);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)))
    {
        perror("");
        exit(0);
    }
    listen(server_fd, 128);
    epoll_fd = epoll_create(1);
    server_event.events = EPOLLIN;
    server_event.data.fd = server_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &server_event);
    addrlen = sizeof(address);

}

Client::Client()
{
    request_received = false;
    response_sent = false;
}

bool Client::read_from_fd(int client_fd)
{
    std::cout << "read from fd" << std::endl;
    char recv_buffer[1024];

    ssize_t n = recv(client_fd, recv_buffer, 1024, 0);
    if (n <= 0)
        return false;

    read_buffer += std::string(recv_buffer, n);
    if (read_buffer.find("\r\n\r\n") != std::string::npos)
    {
        // Header complete
    }

    return true;
}



// bool Client::write_to_fd(int client_fd)
// {
//     std::cout << "read from fd" << std::endl;
//     size_t n = 0;
//     n = read(client_fd , read_buffer, 1024);
// }