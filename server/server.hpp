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

#include "../config_file/parser.hpp"


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


enum RequestParseStatus
{
    PARSE_INCOMPLETE = 1000, // Still receiving data
    PARSE_OK = 200,        // Full and valid request
    PARSE_BAD_REQUEST = 400, // Syntax or structure error
    PARSE_NOT_IMPLEMENTED = 501, // Unsupported method
    REQUEST_METHOD_NOT_ALLOWED = 405, // Method Not Allowed
    PARSE_INTERNAL_ERROR = 500,  // Logic/internal crash
    PARSE_CONNECTION_CLOSED = 0,  // Client closed
    REQUEST_URI_TOO_LONG = 414,    // URI Too Long
    PAYLOAD_TOO_LARGE  = 413 // request entity was larger than limits 
    // REQUEST_NOT_FOUND = 404,       // Not Found
};


class Body {
    public:
        std::string raw_body;
        std::string data;
        std::string _body;
        size_t expected_size = 0;
        bool chunked = false;

        bool chunk_done = false;
        bool is_done = false;
        size_t current_chunk_size;
        bool reading_chunk_size = true;
        void append(const std::string& new_data) 
        {
            data += new_data;
        }
        void reset() 
        {
            data.clear();
            raw_body.clear();
            expected_size = 0;
            chunked = false;
            is_done = false;
            current_chunk_size = 0;
            reading_chunk_size = true;
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


class Client
{
    public:
        Client();
        // Server config;
        long long client_max_body_size;
        std::vector<std::string> allowed_methods;
        HandleReq Hreq;
        // HandleRes Hres;
        std::string read_buffer;
        std::string write_buffer;
        bool request_received;
        bool response_sent;
        std::string file_path;

        RequestParseStatus read_from_fd(int client_fd, size_t max_body_size);
        bool write_to_fd(int client_fd);
        std::string trim(std::string str);
        std::vector<std::string> split( std::string& s, std::string& delimiter);
        bool check_methods();
        void test_chunked_parsing();
        void handle_chunked_body(size_t max_body_size);
        void reset_for_next_request();
        // ~Client();
        std::string response_buffer;
        bool response_ready;
        size_t status_code;       
        void prepare_response() 
        {
            response_buffer =
                "HTTP/1.1 200 OK\r\n"
                "Content-Length: 13\r\n"
                "Connection: keep-alive\r\n"
                "\r\n"
                "Hello, world!";
            response_ready = true;
        }

        // methods
        bool isGET;
        bool isPOST;
        bool isDELETE;
        void getMethod();
        void deleteMethod();
        //stats
        RequestParseStatus status;

};


class Server 
{
    private:
        int type;
        int protocol;
    public:
        
        // std::vector<Location> locations;
        
        ServerCo config;
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
        // Server(int domain, int type, int protocol);
        // Server(const std::string& host, int port);
        Server(const ServerCo& conf);
        // int getServerFd();
        // int getEpollFd();
};


void start_server(std::vector<ServerCo> &servers);

void start_server();
#endif