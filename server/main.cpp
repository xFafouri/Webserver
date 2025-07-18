#include "server.hpp"
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <utility>




int main()
{
    Server sock(AF_INET, SOCK_STREAM, 0);
    while(1)
    {
        size_t i = epoll_wait(sock.epoll_fd,sock.events, 1024, -1);
        // check
        for (int j = 0; j < i; j++)
        {
            if (sock.events[j].data.fd == sock.server_fd)
            {
                sock.client_fd = accept(sock.server_fd, (struct sockaddr*)&sock.address, &sock.addrlen);
                Client *A = new Client;
                sock.sock_map.insert(std::make_pair(sock.client_fd,A));
                sock.client_events.events = EPOLLIN | EPOLLOUT;
                sock.client_events.data.fd = sock.client_fd;
                epoll_ctl(sock.epoll_fd, EPOLL_CTL_ADD, sock.client_fd, &sock.client_events);
                // insert the client in the map 

            }
            // else if (//client does not in the map)
                // check client if in the map accept() 
                // continue 
            else  {
                if (sock.events[j].events & EPOLLIN)
                {
                    // read from fd function called
                    int fd = sock.events[j].data.fd;
                    Client* A = sock.sock_map[fd];
                    if (!A->read_from_fd(fd))
                    {
                        std::cout << "TEST" << std::endl;
                        close(fd);
                        delete A;
                        sock.sock_map.erase(fd);
                        epoll_ctl(sock.epoll_fd, EPOLL_CTL_DEL, fd, &sock.client_events);
                    }
                    epoll_ctl(sock.epoll_fd, EPOLL_CTL_MOD, fd, &sock.client_events);
                }
                if (sock.events[j].events & EPOLLOUT)
                {
                    // write to fd function called
                    int fd = sock.events[j].data.fd;
                    Client* A = sock.sock_map[fd];
                    if (!A->write_to_fd(fd))
                    {
                        close(fd);
                        delete A;
                        sock.sock_map.erase(fd);
                        epoll_ctl(sock.epoll_fd, EPOLL_CTL_DEL, fd, &sock.client_events);
                    }
                    epoll_ctl(sock.epoll_fd, EPOLL_CTL_MOD, fd, &sock.client_events);
                }
                // check for the client event after the accept if EPOLLIN read() or  EPOLLOUT recv()
                // sock.sock_map.insert(std::make_pair(client,A));   
            }
        }
    }
    return 0;
}
