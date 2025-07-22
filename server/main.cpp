#include "server.hpp"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>



void cleanup_connection(Server& sock, int fd) 
{
    close(fd);
    delete sock.sock_map[fd];
    sock.sock_map.erase(fd);
    epoll_ctl(sock.epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
}

bool is_connection_closed(int fd) 
{
    int error = 0;
    socklen_t errlen = sizeof(error);
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &errlen) == -1 || error != 0) {
        return true;
    }
    return false;
}

void cleanup_connection(Server& sock, int fd, bool remove_file = false) 
{
    if (remove_file && sock.sock_map[fd]) {
        std::remove(sock.sock_map[fd]->file_path.c_str());
    }
    close(fd);
    delete sock.sock_map[fd];
    sock.sock_map.erase(fd);
    epoll_ctl(sock.epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
}

int main()
{
    Server sock(AF_INET, SOCK_STREAM, 0);
    while (1)
    {
        int num_events = epoll_wait(sock.epoll_fd, sock.events, 1024, 4000);
        // if (num_events == -1) 
        // {
        //     if (errno != EINTR) {
        //         perror("epoll_wait");
        //     }
        //     continue;
        // }

        for (int j = 0; j < num_events; j++)
        {
            int fd = sock.events[j].data.fd;
            Client* A = sock.sock_map[fd];

            // Handle new connections
            if (fd == sock.server_fd)
            {
                sock.client_fd = accept(sock.server_fd, (struct sockaddr*)&sock.address, &sock.addrlen);
                if (sock.client_fd == -1) {
                    perror("accept");
                    continue;
                }
                
                fcntl(sock.client_fd, F_SETFL, O_NONBLOCK);

                Client* new_client = new Client;
                sock.sock_map[sock.client_fd] = new_client;

                struct epoll_event ev;
                ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
                ev.data.fd = sock.client_fd;
                if (epoll_ctl(sock.epoll_fd, EPOLL_CTL_ADD, sock.client_fd, &ev) == -1) 
                {
                    perror("epoll_ctl ADD");
                    cleanup_connection(sock, sock.client_fd, true);
                }
                continue;
            }

            // Handle errors
            if (sock.events[j].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) 
            {
                cleanup_connection(sock, fd, true);
                continue;
            }

            // Handle read events
            if (sock.events[j].events & EPOLLIN)
            {
                bool connection_ok = true;
                bool request_complete = false;

                while (true)
                {
                   ssize_t bytes_read = A->read_from_fd(fd);
                    if (!bytes_read && A->request_received)
                    {
                        request_complete = true;
                        break;
                    }
                    if (bytes_read < 0)
                    {
                        if ((errno == EAGAIN || errno == EWOULDBLOCK) && A->request_received)
                        {
                            request_complete = true;
                            break;
                        }
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break;

                        connection_ok = false;
                        break;
                    }
                    else if (bytes_read == 0)
                    {
                        std::cout << "DEBUG: read_from_fd indicates connection closed\n";

                        // 🛠️🟢 Check if request was fully received *before* closing
                        if (A->request_received)
                        {
                            request_complete = true;
                        }
                        else
                        {
                            connection_ok = false;
                        }
                        break;
                    }
                    else
                    {
                        if (A->request_received)
                        {
                            request_complete = true;
                            break;
                        }
                    }
                }
                if (!connection_ok)
                {
                    cleanup_connection(sock, fd, true);
                    continue;
                }

               if (request_complete)
                {
                    std::cout << "Request complete, preparing response..." << std::endl;
                    A->prepare_response();

                    struct epoll_event ev;
                    ev.events = EPOLLOUT | EPOLLET | EPOLLRDHUP;
                    ev.data.fd = fd;
                    if (epoll_ctl(sock.epoll_fd, EPOLL_CTL_MOD, fd, &ev) == -1)
                    {
                        perror("epoll_ctl MOD");
                        cleanup_connection(sock, fd, true);
                    }
                    continue;
                }
            }

            // Handle write events
            if (sock.events[j].events & EPOLLOUT) 
            {
                std::cout << "EPOLLOUT event for fd: " << fd << std::endl;
                bool write_complete = false;
                bool connection_ok = true;
                
                do {
                    write_complete = A->write_to_fd(fd);
                    
                    if (!write_complete) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) {
                            std::cout << "Partial write, will continue later" << std::endl;
                            break;
                        }
                        std::cerr << "Write error: " << strerror(errno) << std::endl;
                        connection_ok = false;
                        break;
                    }
                } while (!write_complete);

                if (!connection_ok) 
                {
                    cleanup_connection(sock, fd, true);
                }
                else if (write_complete) 
                {
                    std::cout << "Response fully sent to fd: " << fd << std::endl;
                    if (A->Hreq.header.map_header["Connection"] == "close") 
                    {
                        cleanup_connection(sock, fd, true);
                    }
                    
                    else {
                        // Switch back to reading mode
                        A->reset_for_next_request();
                        struct epoll_event ev;
                        ev.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
                        ev.data.fd = fd;
                        if (epoll_ctl(sock.epoll_fd, EPOLL_CTL_MOD, fd, &ev) == -1) {
                            perror("epoll_ctl MOD");
                            cleanup_connection(sock, fd, true);
                        }
                    }
                }
            }
        }
    }
    return 0;
}