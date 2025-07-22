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
        // bool needs_more_data() const 
        // {
        //     return !complete && (!chunked && data.size() < expected_size);
        // }

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
        HandleReq Hreq;
        // HandleRes Hres;
        std::string read_buffer;
        std::string write_buffer;
        bool request_received = false;;
        bool response_sent;
        std::string file_path;
        size_t read_from_fd(int client_fd);
        bool write_to_fd(int client_fd);
        std::string trim(std::string str);
        std::vector<std::string> split( std::string& s, std::string& delimiter);
        bool check_methods();
        void test_chunked_parsing();
        void handle_chunked_body();
        void reset_for_next_request();
        // ~Client();
        std::string response_buffer;
        bool response_ready;       
        
        // 
        
        void prepare_response() {
            response_buffer =
                "HTTP/1.1 200 OK\r\n"
                "Content-Length: 13\r\n"
                "Connection: keep-alive\r\n"  // <-- This is critical
                "\r\n"
                "Hello, world!";
            response_ready = true;
        }

};


class Server 
{
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