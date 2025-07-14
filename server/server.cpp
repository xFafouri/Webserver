#include "server.hpp"
#include <cstddef>
#include <sstream>
#include <sys/socket.h>



Server::Server(int domain, int type, int protocol)
{
    // domain = AF_INET;
    // type = SOCK_STREAM;
    // protocol = 0;

    server_fd = socket(domain, type, protocol);

    address.sin_family = AF_INET;
    address.sin_port = htons(8081);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)))
    {
        perror("");
        exit(0);
    }
    listen(server_fd, 128);
    epoll_fd = epoll_create(1);
    server_event.events = EPOLLIN;
    server_event.data.fd = server_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &server_event);
    addrlen = sizeof(address);

}

Client::Client()
{
    request_received = false;
    response_sent = false;
}

std::string Client::trim(std::string str)
{
    std::string::iterator start = str.begin();
    while (start != str.end() && std::isspace(*start))
        ++start;
    std::string::iterator end = str.end();
    if (start != str.end())
    {
        --end;
        while (end != start && std::isspace(*end))
            --end;
        return std::string(start, end + 1);
    }
    return "";
}


bool Client::check_methods()
{
    std::string methods[3] = {"GET","POST", "DELETE"};
    for (int i = 0; i < methods->length(); i++)
    {
        if (methods[i] == Hreq.method)
        {
            return true;
        }
    }
    return false;
}

bool Client::read_from_fd(int client_fd)
{
    // char recv_buffer[1024];

    // ssize_t n = read(client_fd, recv_buffer, 1024);
    // std::cout << "read *from fd " << n << std::endl;
    // if (n <= 0)
    //     return ;
    // read_buffer += std::string(recv_buffer, n);
    // if (read_buffer.find("\r\n\r\n") == std::string::npos)
    //     return;
    read_buffer = "sad /index.html HTTP/1.1\r\n"
                    "Host: example.com\r\n"
                    "User-Agent: curl/7.68.0\r\nAccept: */*\r\n"
                    "\r\n"
                    "{\n"
                    "firstName: Brian,\n"
                    "lastName: Smith,\n"
                    "email: bsmth@example.com,\n"
                    "more: data\n"
                    "}\n";
    // std::cout << read_buffer  << std::endl;
    size_t pos = read_buffer.find("\r\n");
    if (pos == std::string::npos)
        return -1;
    std::string request_line = read_buffer.substr(0, pos);
    std::string headers = read_buffer.substr(pos + 2, read_buffer.find("\r\n\r\n") - (pos + 2));
    size_t p = read_buffer.find("\r\n\r\n");

    std::string body = read_buffer.substr(p + 2, read_buffer.size() - 1);


    // std::map<std::string,std::string>::iterator s;
    std::istringstream ss(headers);

    std::string header_line;
    while (std::getline(ss, header_line)) 
    {
        size_t sep = header_line.find(":");
        if (sep != std::string::npos) 
        {
            std::string key = header_line.substr(0, sep);
            std::string value = header_line.substr(sep + 1);
            trim(key);
            trim(value);
            Hreq.map_header.insert(std::make_pair(key, value));
        }
    }


    std::istringstream iss(request_line);

    iss >> Hreq.method >> Hreq.uri >> Hreq.http_v;

    if (!check_methods())
    {
        std::cout << "Error Method" << std::endl;
        return -1;
    }

    //check content-type 

    if (Hreq.map_header.find("Content-Type") != Hreq.map_header.end()) 
    {
       Hreq.content_type = Hreq.map_header["Content-Type"];
    }
    std::istringstream dd(body);
    std::string body_line;

    //
    if (Hreq.method == "POST")
    {
        if (Hreq.map_header.find("Content-Length") != Hreq.map_header.end()) 
        {
            std::string len_str = Hreq.map_header["Content-Length"];
            Hreq.content_length = std::atoi(len_str.c_str());
        }
        // check Transfer-Encoding
    }
    while (std::getline(dd, body_line))
    {
        size_t sep = body_line.find(":");
        if (sep != std::string::npos)
        {
            std::string key = body_line.substr(0, sep);
            // check 
            std::string value = body_line.substr(sep + 1, body_line.find("\n"));
            Hreq.body_map.insert(std::make_pair(key, value));
        }
    }
    // check methods

    std::map<std::string,std::string>::iterator itt = Hreq.body_map.begin();

    // for (;itt != Hreq.body_map.end();itt++)
    // {
    //     std::cout << "key = " << itt->first << " | ";
    //     std::cout << "value = " << itt->second  << std::endl;
    // }

    return true;
}



bool Client::write_to_fd(int fd)
{
    const std::string response =
        "HTTP/1.1 200 OK\r\n"
        "Content-Length: 13\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "Hello, world!";

    ssize_t sent = send(fd, response.c_str(), response.length(), 0);
    if (sent <= 0)
    {
        std::cerr << "Failed to send response or client closed connection\n";
        return false;
    }

    std::cout << "Response sent. Closing connection.\n";
    return false; 
}
