#include "server.hpp"
#include <cstdlib>
#include <map>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#include <utility>
#include <csignal>

volatile sig_atomic_t stop_flag = 0;

void handle_sigint(int) {
    stop_flag = 1; // just set a flag
}

void cleanup_connection(Server& sock, int fd) 
{
    close(fd);
    delete sock.sock_map[fd];
    sock.sock_map.erase(fd);
    epoll_ctl(sock.epoll_fd, EPOLL_CTL_DEL, fd, NULL);
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
    (void) remove_file;
    close(fd);
    delete sock.sock_map[fd];
    sock.sock_map.erase(fd);
    epoll_ctl(sock.epoll_fd, EPOLL_CTL_DEL, fd, NULL);
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

void check_timeouts(std::vector<Server>& servers, int global_epoll) 
{
    int64_t now = Client::now_ms();
    const int64_t CLIENT_IDLE_MS = 15000;

    for (size_t si = 0; si < servers.size(); ++si) 
    {
        Server &srv = servers[si];
        std::vector<int> to_close_fds;

        for (std::map<int, Client*>::iterator it = srv.sock_map.begin();
            it != srv.sock_map.end(); ++it)
        {
            int fd = it->first;
            Client *client = it->second;
            if (!client->is_cgi) 
            {
                if (now - client->last_activity > CLIENT_IDLE_MS) 
                {
                    // std::cout << "[Timeout] Idle client fd=" << fd << std::endl;
                    client->sendError(408);
                    client->timed_out = true;  
                    struct epoll_event ev;
                    ev.events = EPOLLOUT | EPOLLRDHUP;
                    ev.data.fd = fd;
                    if (epoll_ctl(global_epoll, EPOLL_CTL_MOD, fd, &ev) == -1) 
                    {
                        std::cerr << "epoll_ctl MOD EPOLLOUT (timeout): " << strerror(errno) << std::endl;
                        cleanup_connection(srv, fd, true);
                    }
                    continue;
                }
            }
            if (client->is_cgi && (client->cgi_state == CGI_IO || client->cgi_state == CGI_SPAWNED)) 
            {
                uint64_t now = Client::now_ms();
                if ((now > client->cgi_deadline_ms && client->cgi_deadline_ms > 0) || (now - client->cgi_start_time) > 10000)
                {
                    client->sendError(504);
                    client->timed_out = true;

                    if (client->cgi_pid > 0)
                        kill(client->cgi_pid, SIGKILL);

                    client->cgi_state = CGI_ERROR;

                    struct epoll_event ev;
                    ev.events = EPOLLOUT | EPOLLRDHUP;
                    ev.data.fd = fd;
                    if (epoll_ctl(global_epoll, EPOLL_CTL_MOD, fd, &ev) == -1) {
                        perror("epoll_ctl MOD EPOLLOUT (timeout)");
                        cleanup_connection(srv, fd, true);
                    }
                    continue;
                }
            }
        }
        for (size_t i = 0; i < to_close_fds.size(); ++i) 
        {
            int fd = to_close_fds[i]; 
            std::cerr << "--->Timeout] closing fd = " << fd << " on server " << si << "\n";
            cleanup_connection(srv, fd, true);
        }
    } 
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

    std::signal(SIGINT, handle_sigint);
    while (!stop_flag)
    {
        int num_events = epoll_wait(global_epoll, events, 1024, 4000);
        if (num_events == -1) 
        {
            if (errno == EINTR) {
                continue;
            }
            perror("epoll_wait");
            continue;
        }

        for (int i = 0; i < num_events; ++i)
        {
            int fd = events[i].data.fd;
            uint32_t ev = events[i].events;

            Server* target_server = NULL;
            Client* client = NULL;
            
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
                    client = servers[j].sock_map[fd];
                    client->config = servers[j].config;
                    break;
                }
                if (!client)
                {
                    for (std::map<int, Client*>::iterator it = servers[j].sock_map.begin(); it != servers[j].sock_map.end(); ++it) 
                    {
                        Client* c = it->second;
                        if (c->cgi_state == CGI_IO && (fd == c->cgi_stdout_fd || fd == c->cgi_stdin_fd)) 
                        {
                            target_server = &servers[j];
                            client = c;
                            break;
                        }
                    }
                }
                if (client)
                {
                    break;
                }
            }
            
            if (!target_server) 
                continue;

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
                new_client->last_activity = new_client->now_ms();

                struct epoll_event ev;
                ev.events = EPOLLIN | EPOLLRDHUP;
                ev.data.fd = client_fd;
                if (epoll_ctl(global_epoll, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
                    perror("epoll_ctl ADD client");
                    cleanup_connection(*target_server, client_fd, true);
                }
                continue;
            }
            else if (client && (fd == client->cgi_stdout_fd || fd == client->cgi_stdin_fd)) 
            {
                client->on_cgi_event(global_epoll, fd, ev);
                client->last_activity = client->now_ms();
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
                continue;
            }
            else if (events[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) 
            {
                cleanup_connection(*target_server, fd, true);
                continue;
            }
            if (events[i].events & EPOLLIN)
            {
                bool connection_ok = true;
                bool request_complete = false;
                client->last_activity = client->now_ms();
                client->status = client->read_from_fd(fd, target_server->config.client_max_body_size);
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
                    
                        client->sendError(client->status);
                        request_complete = true;
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
                        client->is_cgi = true;
                        if (client->run_cgi(global_epoll))
                        {
                            continue;
                        }
                        else 
                        {
                            client->prepare_response();
                        }
                    }
                    else {
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
            
            if (events[i].events & EPOLLOUT)
            {
                // std::cout << "response buffer = " << client->response_buffer << std::endl;
                bool write_complete = client->write_to_fd(fd);
               
                if (write_complete)
                {
                    client->Hreq.header.map_header["Connection"] = "close";
                    if (client->timed_out)
                    {
                        // std::cout << "[Timeout] Response sent, closing fd=" << fd << std::endl;
                        cleanup_connection(*target_server, fd, true);
                    } 
                    else if (client->Hreq.header.map_header["Connection"] == "close")
                    {
                        cleanup_connection(*target_server, fd, true);
                    }
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
        check_timeouts(servers, global_epoll);
    }
        for (size_t i = 0; i < servers.size(); ++i) {
        Server& server = servers[i];
        for (std::map<int, Client*>::iterator it = server.sock_map.begin(); it != server.sock_map.end(); ++it) {
            close(it->first);
            delete it->second;
        }
        server.sock_map.clear();
        if (server.server_fd > 0) close(server.server_fd);
        if (server.epoll_fd > 0) close(server.epoll_fd);
    }
}