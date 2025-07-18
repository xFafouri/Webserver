#ifndef SERVER_HPP
#define SERVER_HPP

#include <codecvt>
#include <cstddef>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <unistd.h>
#include <vector>

#include <algorithm>
#include <sstream>
#include <fcntl.h>
#include <stdexcept>
#include <string>
#include <sys/types.h>

class Header
{
    public :
        std::string method;
        std::string uri;
        std::string http_v;
        std::map<std::string,std::string> map_header;
        std::string content_type;
        bool parsed = false;
};

class Body 
{
    public:
        std::string data;
        size_t expected_size = 0;
        bool complete = false;
        bool chunked = false;
        size_t chunk_pos = 0;

        void append(const std::string& new_data) 
        {
            data += new_data;
        }
};

class HandleReq 
{
    public:
        // req line
        Header header;
        Body body;
        std::string method;
        std::string uri;
        std::string http_v;
        std::string buffer;

        std::map<std::string,std::string> map_header;
        std::map<std::string,std::string> body_map;
        // Headers
        // std::map<std::string,std::string> Header;
        // Body
        size_t content_length;
        std::string content_type;
        std::string transfer_encoding;

};

class HandleRes
{

};



class Client 
{
    public:
        Client();
        HandleReq Hreq;
        HandleRes Hres;
        std::string read_buffer;
        std::string write_buffer;
        bool request_received = false;;
        bool response_sent;
        std::string file_path;
        bool read_from_fd(int client_fd);
        bool write_to_fd(int client_fd);
        std::string trim(std::string str);
        std::vector<std::string> split( std::string& s, std::string& delimiter);
        bool check_methods();

        // ~Client();

};


class Server {
    private:
    int type;
    int protocol;
    
    public:
        std::map<int, Client *> sock_map;
        struct sockaddr_in address;
        socklen_t addrlen;
        // struct sockaddr add_accept;
        int server_fd;
        int client_fd;
        int epoll_fd;
        epoll_event server_event;
        epoll_event client_events;
        epoll_event events[1024];
        Server(int domain, int type, int protocol);
        // int getServerFd();
        // int getEpollFd();
};


#endif