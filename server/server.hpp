#ifndef SERVER_HPP
#define SERVER_HPP


#define RESET       "\033[0m"
#define BLACK       "\033[30m"
#define RED         "\033[31m"
#define GREEN       "\033[32m"
#define YELLOW      "\033[33m"
#define BLUE        "\033[34m"
#define MAGENTA     "\033[35m"
#define CYAN        "\033[36m"
#define WHITE       "\033[37m"


#include <cstddef>
#include <cstring>
#include <fstream>
#include <iostream>
#include <map>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <vector>
#include <strings.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#include <algorithm>
#include <sstream>
#include <fcntl.h>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <sys/wait.h>

#include <sys/types.h>
#include <unistd.h>

#include <cstddef>
#include <cstdlib>
#include <exception>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/stat.h>
#include <dirent.h>
#include <iomanip>
#include <cctype>

#include "../config_file/parser.hpp"

#include <sys/time.h>
#include <stdint.h>



class Header
{
    public :
        std::string method;
        std::string uri;
        std::string http_v;
        std::map<std::string,std::string> map_header;
        std::string content_type;
        bool parsed;
};


enum CgiState 
{
    CGI_IDLE = 0,
    CGI_SPAWNED,
    CGI_IO,
    CGI_DONE,
    CGI_ERROR,
    CGI_TIMED_OUT
};

enum RequestParseStatus
{
    PARSE_INCOMPLETE = 1000, // Still receiving data
    PARSE_FORBIDDEN = 403,
    PARSE_OK = 200,        // Full and valid request
    PARSE_BAD_REQUEST = 400, // Syntax or structure error
    PARSE_NOT_IMPLEMENTED = 501, // Unsupported method
    REQUEST_METHOD_NOT_ALLOWED = 405, // Method Not Allowed
    PARSE_INTERNAL_ERROR = 500,  // Logic/internal crash
    PARSE_CONNECTION_CLOSED = 0,  // Client closed
    REQUEST_URI_TOO_LONG = 414,    // URI Too Long
    PAYLOAD_TOO_LARGE  = 413, // request entity was larger than limits 
    REQUEST_NOT_FOUND = 404     // Not Found
};


class Body 
{
    public:
        std::string raw_body;
        std::string data;
        std::string _body;
        size_t expected_size;
        bool chunked;

        bool chunk_done;
        bool is_done;
        size_t current_chunk_size;
        bool reading_chunk_size;
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
        HandleReq Hreq;
        std::string file_path;
        template <typename T>
        std::string to_string98(T value) 
        {
            std::ostringstream oss;
            oss << value;
            return oss.str();
        }


        // cgi
        void cleanup_cgi_fds(int epoll_fd);

        bool is_valid_fd(int fd);

        pid_t       cgi_pid;
        int         cgi_stdin_fd;
        int         cgi_stdout_fd;
        size_t      cgi_stdin_off;
        CgiState    cgi_state;
        int cgi_error_code;

        std::string cgi_raw;
        bool        cgi_hdr_parsed;
        size_t      cgi_sep_pos;
        size_t      cgi_sep_len;

        // Parsed header cgi
        std::string cgi_status_line;
        std::string cgi_content_type;
        ssize_t     cgi_content_len;

        // Deadline
        uint64_t    cgi_deadline_ms;
        std::string cl;
        static uint64_t now_ms()
        {
            struct timeval tv;
            gettimeofday(&tv, NULL);
            return (uint64_t)tv.tv_sec * 1000 + (uint64_t)tv.tv_usec / 1000;
        }
        int64_t last_activity;
        int64_t cgi_start_time;

        void    refresh_deadline();
        bool    is_cgi_script(const std::string &path);
        bool    is_cgi_request();
        bool    run_cgi(int epoll_fd);
        void    finalize_cgi(bool eof_seen);
        void    on_cgi_event(int epoll_fd, int fd, uint32_t events);
        void    abort_cgi(int epoll_fd);
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
        bool is_sending_response;
        size_t last_write_time;
        void sendTimeoutResponse();

        // methods
        bool isGET;
        bool isPOST;
        bool isDELETE;
        void getMethod();
        void deleteMethod();
        std::string ft_content_type(const std::string& full_path);
        std::string normalize_path(const std::string &path);
        // add another funtion
        std::string ft_extension_from_type(const std::string& content_type);
        std::string ft_file_extension(const std::string& full_path);

        //stats
        RequestParseStatus status;
        size_t status_code;    

        //timeout 
        int to_close;
        int id_server;
        bool timed_out;
        

};


class Server 
{
    public:
                
        ServerCo config;
        std::map<int, Client *> sock_map;
        // struct sockaddr_in address;
        struct addrinfo address;
        struct addrinfo *res;
        socklen_t addrlen;
        // struct sockaddr add_accept;
        int server_fd;
        int client_fd;
        int epoll_fd;
        epoll_event server_event;
        epoll_event client_events;
        epoll_event events[1024];
        Server(const ServerCo& conf);
         template <typename T>
        std::string to_string(T value) 
        {
            std::ostringstream oss;
            oss << value;
            return oss.str();
        }

};


void start_server(std::vector<ServerCo> &servers);

void start_server();
#endif
