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
#include <sys/wait.h>


#include <cstddef>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>

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
        void append(const std::string& new_data);
        void reset();
};



class HandleReq 
{
    public:
        // req line
        std::string method;
        std::string uri;
        std::string http_v;
        std::string buffer;
        //Header
        Header header;
        std::map<std::string,std::string> map_header;
        //Body
        Body body;
        std::map<std::string,std::string> body_map;
        size_t content_length;
        std::string content_type;
        std::string transfer_encoding;

};


class Client
{
    public:
        Client();
        ServerCo config;
        long long client_max_body_size;
        std::vector<std::string> allowed_methods;
        // std::vector<Location> locations;
        HandleReq Hreq;
        // HandleRes Hres;
        std::string file_path;
        

        // cgi 
        bool is_cgi_script(const std::string &path);
        bool is_cgi_request();
        void    run_cgi();
        int location_idx;
        bool cgi_bin;
        std::map<std::string , std::string> env_map;
        std::map<std::string , std::string> map_ext;
        std::string extension;
        std::string script_file;
        bool is_cgi;

        //request 
        bool request_received;
        std::string read_buffer;
        RequestParseStatus read_from_fd(int client_fd, long long max_body_size);
        std::string trim(std::string str);
        std::vector<std::string> split( std::string& s, std::string& delimiter);
        bool check_methods();
        void test_chunked_parsing();
        void handle_chunked_body(size_t max_body_size);
        void reset_for_next_request();
        
        //response 
        std::string write_buffer;
        bool response_sent;
        std::string response_buffer;
        bool write_to_fd(int client_fd);
        bool response_ready;
        int client_fd;
        void prepare_response();
        ssize_t send_offset;
        bool header_sent;
        ssize_t header_size;
        ssize_t total;
        ssize_t remaining;
        ssize_t send_len;
        void sendError(int code);


        // methods
        bool isGET;
        bool isPOST;
        bool isDELETE;
        void getMethod();
        void deleteMethod();
        std::string ft_content_type(const std::string& full_path);
        std::string normalize_path(const std::string &path);


        //stats
        RequestParseStatus status;
        size_t status_code;       

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