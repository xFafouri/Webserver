#include "server.hpp"
#include <map>
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
    if (getsockopt(fd, SOL_SOCKET, SO_ERROR, &error, &errlen) == -1 || error != 0) 
    {
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


void start_server(std::vector<ServerCo>& configs)
{
    std::vector<Server> servers;

    for (size_t i = 0; i < configs.size(); ++i)
    {
        try {
            servers.push_back(Server(configs[i]));
        } catch (const std::exception& e) {
            std::cerr << "Server init failed: " << e.what() << std::endl;
        }
    }

    int global_epoll = epoll_create(1);
    struct epoll_event events[1024];

    for (size_t i = 0; i < servers.size(); ++i)
    {
        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLRDHUP;
        ev.data.fd = servers[i].server_fd;

        if (epoll_ctl(global_epoll, EPOLL_CTL_ADD, servers[i].server_fd, &ev) == -1)
        {
            perror("epoll_ctl ADD server_fd");
            exit(1);
        }
    }

    while (true)
    {
        int num_events = epoll_wait(global_epoll, events, 1024, 4000);
        if (num_events == -1) 
        {
            perror("epoll_wait");
            continue;
        }
        for (int i = 0; i < num_events; ++i)
        {
            int fd = events[i].data.fd;
            uint32_t ev = events[i].events;

            Server* target_server = nullptr;
            Client* client = nullptr;
            
            for (size_t j = 0; j < servers.size(); ++j)
            {
                if (fd == servers[j].server_fd) 
                {
                    target_server = &servers[j];
                    break;
                }
                if (servers[j].sock_map.count(fd))
                {
                    target_server = &servers[j];
                    std::cout << "***" << std::endl;
                    // target_server->
                    client = servers[j].sock_map[fd];
                    // client->locations = target_server->config.locations;
                    client->config = servers[j].config;
                    // client->allowed_methods = target_server->config.locations[j].allowed_methods;
                    break;
                }
                // check CGI fds
                for (std::map<int, Client*>::iterator it = servers[j].sock_map.begin(); it != servers[j].sock_map.end(); ++it) 
                {
                    Client* c = it->second;
                    if (c->cgi_state == CGI_IO && (fd == c->cgi_stdout_fd || fd == c->cgi_stdin_fd)) {
                        target_server = &servers[j];
                        client = c;
                        break;
                    }
                }
                if (client) 
                    break;
            }
            

            if (!target_server || !client) 
                continue;

            // --- CASE 1: server socket ---
            if (fd == target_server->server_fd)
            {
                int client_fd = accept(fd, (sockaddr*)&target_server->address, &target_server->addrlen);
                if (client_fd == -1) 
                {
                    perror("accept");
                    continue;
                }

                fcntl(client_fd, F_SETFL, O_NONBLOCK);
                Client* new_client = new Client;
                target_server->sock_map[client_fd] = new_client;

                struct epoll_event ev;
                ev.events = EPOLLIN | EPOLLRDHUP;
                ev.data.fd = client_fd;
                if (epoll_ctl(global_epoll, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
                    perror("epoll_ctl ADD client");
                    cleanup_connection(*target_server, client_fd, true);
                }
                continue;
            }

            // --- CASE 2: CGI fd ---
            if (fd == client->cgi_stdout_fd || fd == client->cgi_stdin_fd) 
            {
                client->on_cgi_event(global_epoll, fd, ev);
                continue;
            }

            // --- CASE 3: normal client socket ---
            if (events[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) 
            {
                cleanup_connection(*target_server, fd, true);
                continue;
            }
            //handle read
            if (events[i].events & EPOLLIN)
            {
                bool connection_ok = true;
                bool request_complete = false;

                client->status = client->read_from_fd(fd, target_server->config.client_max_body_size);
                std::cout << "status = " << client->status << std::endl;
                if (client->status == PARSE_OK) 
                {
                    request_complete = true;
                } 
                else if (client->status == PARSE_INCOMPLETE) 
                {
                    request_complete = false;
                } 
                else if (client->status == PARSE_CONNECTION_CLOSED) 
                {
                    connection_ok = false;
                } 
                else 
                {
                    connection_ok = false;
                }

                if (!connection_ok) 
                {
                    cleanup_connection(*target_server, fd, true);
                    continue;
                }
                    
                if (request_complete && client->is_cgi)
                {
                    if (!client->run_cgi(global_epoll, 5000)) 
                    {
                        client->response_ready = true;
                        epoll_event ev; ev.events = EPOLLOUT | EPOLLRDHUP; ev.data.fd = fd;
                        epoll_ctl(global_epoll, EPOLL_CTL_MOD, fd, &ev);
                        
                    } 
                    else 
                    {
                        client->cgi_state = CGI_IO;
                    }
                    continue;
                }
                if (request_complete) 
                {
                    // std::cout << "Read_buffer = " << client->read_buffer << std::endl;
                    client->prepare_response();
                    struct epoll_event ev;
                    ev.events = EPOLLOUT | EPOLLRDHUP;
                    ev.data.fd = fd;
                    if (epoll_ctl(global_epoll, EPOLL_CTL_MOD, fd, &ev) == -1) {
                        perror("epoll_ctl MOD EPOLLOUT");
                        cleanup_connection(*target_server, fd, true);
                    }
                }
            }
            // handle write
            if (events[i].events & EPOLLOUT)
            {
                bool write_complete = client->write_to_fd(fd);

                if (write_complete)
                {
                    // client->Hreq.header.map_header["Connection"] = "close";
                    if (client->Hreq.header.map_header["Connection"] == "close")
                        cleanup_connection(*target_server, fd, true);
                    else
                    {
                        client->reset_for_next_request();
                        struct epoll_event ev;
                        ev.events = EPOLLIN | EPOLLRDHUP;
                        ev.data.fd = fd;
                        if (epoll_ctl(global_epoll, EPOLL_CTL_MOD, fd, &ev) == -1)
                        {
                            perror("epoll_ctl MOD EPOLLIN");
                            cleanup_connection(*target_server, fd, true);
                        }
                    }
                }
            }

        }
    }
}
