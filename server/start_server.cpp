#include "server.hpp"
#include <cstdlib>
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

void cleanup_invalid_client(Server& server, int fd, bool delete_client, int global_epoll)
{
    epoll_ctl(global_epoll, EPOLL_CTL_DEL, fd, NULL);
    
    close(fd);
    
    if (server.sock_map.count(fd)) 
    {
        Client* client = server.sock_map[fd];
        client->cleanup_cgi_fds(global_epoll);
        if (delete_client) {
            delete client;
            server.sock_map.erase(fd);
        }
    }
}
void check_cgi_timeouts(std::vector<Server>& servers, int epoll_fd)
{
    long long current_time = Client::now_ms();
    
    for (size_t i = 0; i < servers.size(); ++i)
    {
        std::vector<int> to_remove;
        
        for (std::map<int, Client*>::iterator it = servers[i].sock_map.begin(); 
             it != servers[i].sock_map.end(); ++it) 
        {
            Client* client = it->second;
            std::cout << "CGI STATS = "<< client->cgi_state << std::endl;
            if ((client->cgi_state == CGI_IO || client->cgi_state == CGI_SPAWNED) &&
                client->cgi_deadline_ms > 0 && current_time > client->cgi_deadline_ms)
            {
                std::cout << "CGI timeout detected for client fd: " << it->first << std::endl;
                client->abort_cgi(epoll_fd);
                client->response_ready = true;
                
                // Add timeout response
                const char* msg = "Gateway Timeout";
                std::ostringstream oss;
                oss << "HTTP/1.1 504 Gateway Timeout\r\n"
                    << "Content-Type: text/plain\r\n"
                    << "Content-Length: " << strlen(msg) << "\r\n"
                    << "\r\n"
                    << msg;
                client->response_buffer = oss.str();
                
                // Mark for removal if client fd is invalid
                if (!client->is_valid_fd(it->first)) 
                {
                    to_remove.push_back(it->first);
                }
            }
        }
        
        // Clean up invalid clients
        for (size_t j = 0; j < to_remove.size(); ++j) {
            cleanup_invalid_client(servers[i], to_remove[j], true, epoll_fd);
        }
    }
}

// void check_cgi_timeouts(std::vector<Server>& servers, int epoll_fd)
// {
//     // long long now_ms = Client::now_ms();
//     long long current_time = Client::now_ms();
//     int fd = -1;
//     for (size_t i = 0; i < servers.size(); ++i)
//     {
//         std::vector<int> to_remove;
        
//         for (std::map<int, Client*>::iterator it = servers[i].sock_map.begin(); 
//              it != servers[i].sock_map.end(); ++it) 
//         {
//             Client* client = it->second;
//             if ((client->cgi_state == CGI_IO || client->cgi_state == CGI_SPAWNED) &&
//                 client->cgi_deadline_ms > 0 && current_time > client->cgi_deadline_ms)
//             {
//                 fd = client->client_fd;
//                 client->abort_cgi(epoll_fd);
//                 client->response_ready = true;
                
//                 // Mark for removal if client fd is invalid
//                 if (!client->is_valid_fd(it->first)) 
//                 {
//                     to_remove.push_back(it->first);
//                 }
//             }
//         }
        
//         // Clean up invalid clients
//         for (size_t j = 0; j < to_remove.size(); ++j) {
//             cleanup_invalid_client(servers[i], fd,true , epoll_fd);
//         }
//     }
// }

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
    if (global_epoll == -1) 
    {
        perror("epoll_create");
        exit(1);
    }

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

        check_cgi_timeouts(servers, global_epoll);
        
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
                    std::cout << "server socket" << std::endl;
                    break;
                }
                if (servers[j].sock_map.count(fd))
                {
                    
                    target_server = &servers[j];
                    // target_server->
                    client = servers[j].sock_map[fd];
                    // client->locations = target_server->config.locations;
                    client->config = servers[j].config;
                    std::cout << "client socket" << std::endl;
                    // client->allowed_methods = target_server->config.locations[j].allowed_methods;
                    break;
                }
                // check CGI fds
                if (!client)
                {
                    for (std::map<int, Client*>::iterator it = servers[j].sock_map.begin(); it != servers[j].sock_map.end(); ++it) 
                    {
                        Client* c = it->second;
                        if (c->cgi_state == CGI_IO && (fd == c->cgi_stdout_fd || fd == c->cgi_stdin_fd)) 
                        {
                            target_server = &servers[j];
                            client = c;
                            std::cout << "CGI fds" << std::endl;
                            break;
                        }
                    }
                }
                if (client)
                {
                    std::cout << "client null" << std::endl;
                    break;
                }
            }
            
            // if (!target_server || !client) 
            //     continue;

            if (!target_server) 
                continue;

            if (fd == target_server->server_fd)
            {
                std::cout << "server socket" << std::endl;
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

            //handle cgi event
            else if (client && (fd == client->cgi_stdout_fd || fd == client->cgi_stdin_fd)) 
            {
                std::cout << "fd =  " << fd << std::endl;
                std::cout << "client =  " << client << std::endl;
                std::cout << "CGI FD" << std::endl;
                client->on_cgi_event(global_epoll, fd, ev);
                if (client->cgi_state == CGI_DONE || client->cgi_state == CGI_ERROR)
                {
                    client->response_ready = true;
                    // std::cout << "response buffer = " << client->response_buffer << std::endl;
                    struct epoll_event ev_out;
                    ev_out.events = EPOLLOUT | EPOLLRDHUP;
                    ev_out.data.fd = client->client_fd;
                    if (epoll_ctl(global_epoll, EPOLL_CTL_MOD, client->client_fd, &ev_out) == -1) 
                    {
                        perror("epoll_ctl MOD EPOLLOUT for CGI");
                    }
                }
                // std::cout << "response buffer = " << client->response_buffer << std::endl;
                std::cout << "cgi stats = " << client->is_cgi << std::endl;
                continue;
            }
            else if (events[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) 
            {
                std::cout << "normal client socket " << std::endl;
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
                if (request_complete)
                {
                    if (client->is_cgi) 
                    {
                        std::cout << "cgi request" << std::endl;
                        client->is_cgi = true;
                        if (client->run_cgi(global_epoll, 5000 )) 
                        {
                            continue;
                        }
                        else 
                        {
                            client->prepare_response();
                        }
                    }
                    else  {
                        client->prepare_response();
                    }
                    struct epoll_event ev_out;
                    ev_out.events = EPOLLOUT | EPOLLRDHUP;
                    ev_out.data.fd = fd;
                    if (epoll_ctl(global_epoll, EPOLL_CTL_MOD, fd, &ev_out) == -1) {
                        perror("epoll_ctl MOD EPOLLOUT");
                        cleanup_connection(*target_server, fd, true);
                    }
                }

            }
            // handle write
            if (events[i].events & EPOLLOUT)
            {
                std::cout << "handle write " << std::endl;
                // std::cout << "response buffer = " << client->response_buffer << std::endl;
                bool write_complete = client->write_to_fd(fd);

                if (write_complete)
                {
                    client->Hreq.header.map_header["Connection"] = "close";
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