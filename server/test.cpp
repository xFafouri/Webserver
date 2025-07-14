#include <algorithm>
#include <cstddef>
#include <iostream>
#include <sstream>
#include "server.hpp"
#include <fcntl.h>
#include <string>
#include <unordered_map>
#include <utility>

#include <fstream>
#include <sstream>
#include <vector>
#include <string>
// bool check_methods()
// {
//     std::string methods[3] = {"GET","POST", "DELETE"};
//     for (int i = 0; i < methods->length(); i++)
//     {
//         if (methods[i] == method)
//         {
//             return true;
//         }
//     }
//     return false;
// }

std::string trim(std::string str)
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

std::vector<std::string> split( std::string& s, std::string& delimiter) 
{
    std::vector<std::string> tokens;
    size_t pos = 0;
    std::string token;
    while ((pos = s.find(delimiter)) != std::string::npos) 
    {
        token = s.substr(0, pos);
        if (token == delimiter + "--")
            break;
        tokens.push_back(token);
        s.erase(0, pos + delimiter.length());
    }
    tokens.push_back(s);

    return tokens;
}

void read_from_fd(int client_fd)
{
    std::string method;
    std::string uri;
    std::string http_v;
    std::map<std::string,std::string> map_header;
    std::map<std::string,std::string> body_map;
    char recv_buffer[1024];
    std::string read_buffer;

    // ssize_t n = read(client_fd, recv_buffer, 1024);
    // std::cout << "read *from fd " << n << std::endl;
    // if (n <= 0)
    //     return ;
    // read_buffer += std::string(recv_buffer, n);
    // if (read_buffer.find("\r\n\r\n") == std::string::npos)
    //     return;

        // ** Json
    // read_buffer =
    //     "POST /index.html HTTP/1.1\r\n"
    //     "Host: example.com\r\n"
    //     "User-Agent: curl/7.68.0\r\n"
    //     "Accept: */*\r\n"
    //     "Content-Type: application/json\r\n"
    //     "Content-Length: 89\r\n"
    //     "\r\n"
    //     "{\n"
    //     "  \"firstName\": \"Brian\",\n"
    //     "  \"lastName\": \"Smith\",\n"
    //     "  \"email\": \"bsmth@example.com\",\n"
    //     "  \"more\": \"data\"\n"
    //     "}";
    
            //**application/x-www-form-urlencoded**
    read_buffer =
        "POST /upload HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "User-Agent: curl/7.68.0\r\n"
        "Accept: */*\r\n"
        "Content-Type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
        "Content-Length: 345\r\n"
        "\r\n"
        "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
        "Content-Disposition: form-data; name=\"firstName\"\r\n"
        "\r\n"
        "Brian\r\n"
        "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
        "Content-Disposition: form-data; name=\"file\"; filename=\"hello.txt\"\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "Hello, this is the content of the file.\r\n"
        "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n";
            // **multipart/form-data**
    




    // std::cout << read_buffer  << std::endl;
    size_t pos = read_buffer.find("\r\n");
    if (pos == std::string::npos)
        return ;
    std::string request_line = read_buffer.substr(0, pos);
    std::string headers = read_buffer.substr(pos + 2, read_buffer.find("\r\n\r\n") - (pos + 2));
    size_t p = read_buffer.find("\r\n\r\n");

    std::string body = read_buffer.substr(p + 4, read_buffer.size() - 1);


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
            key = trim(key);
            value = trim(value);
            map_header.insert(std::make_pair(key, value));
        }
    }

    std::istringstream iss(request_line);

    iss >> method >> uri >> http_v;

    // if (!check_methods())
    // {
    //     std::cout << "Error Method" << std::endl;
    //     return -1;
    // }
    std::istringstream dd(body);

    //check content-type 

    

    int content_length = 0;
    std::string content_type;

    if (map_header.find("Content-Type") != map_header.end()) 
    {
       content_type = map_header["Content-Type"];
    }
    std::cout << "content_type = *" << content_type << "*" << std::endl;
    // check content type / 
    //
    if (method == "POST")
    {
        if (map_header.find("Content-Length") != map_header.end()) 
        {
            std::string len_str = map_header["Content-Length"];
            content_length = std::atoi(len_str.c_str());
        }
        // check Transfer-Encoding
    }
    std::cout << "content_length = " << content_length << std::endl;

    if (content_type.find("application/json") == 0 || content_type == "text/plain")
    {
        std::string file_path = "/tmp/request_client_fd";
        std::ofstream outfile(file_path.c_str());
        if (outfile.is_open()) 
        {
            outfile << body;
            outfile.close();
        }
        // std::string body_line;
        //  while (std::getline(dd, body_line) ) //&& content_length > 0)
        // {
        //     // std::cout << "body line size = " <<  body_line.length() << std::endl;
        //     // content_length = content_length - body_line.length();
        //     // std::cout << "content_length = " << content_length << std::endl;
        //     size_t sep = body_line.find(":");
        //     if (sep != std::string::npos)
        //     {
        //         std::string key = body_line.substr(0, sep);
        //         // check
        //         std::string value = body_line.substr(sep + 1, body_line.find("\n"));
        //         key = trim(key);
        //         value = trim(value);
        //         if (key.front() == '"' && key.back() == '"')
        //         {
        //             key = key.substr(1, key.size() - 2);
        //         }
        //         if (value.front() == '"' && value.back() == ',')
        //         {
        //             value = value.substr(1, value.size() - 2);
        //         }
        //         body_map.insert(std::make_pair(key, value));
        //     }
        // }
    }
    else if (content_type.find("application/x-www-form-urlencoded") == 0)
    {
        std::cout << "urlencoded" << std::endl;
        std::string body_line;
        std::getline(dd, body_line);

        std::stringstream pair_stream(body_line);
        std::string pair;

        while (std::getline(pair_stream, pair, '&'))
        {
            size_t equal_pos = pair.find('=');
            if (equal_pos != std::string::npos)
            {
                std::string key = pair.substr(0, equal_pos);
                std::string value = pair.substr(equal_pos + 1);
                body_map.insert(std::make_pair(key, value));
            }
        }
    }
    else if (content_type.find("multipart/form-data") == 0)
    {

        std::cout << "multipart" << std::endl;
        std::string boundary = "--" + content_type.substr(content_type.find("=") + 1,  content_type.back()) + "\r\n";
        // std::string boundary = content_type.substr(content_type.find("=") + 1);
        // std::string boundary = "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n";
        std::cout << "boundary = " << boundary << std::endl;
        std::cout << "body = " << body << std::endl;
        

        std::vector<std::string> body_vector = split(body, boundary);
        
        std::vector<std::string>::iterator it = body_vector.begin();
        for(;it != body_vector.end(); it++)
        {
            if (*it == "--" || *it == "" || *it == "\r\n\r\n")
                continue;
            if (*it == "Content-Disposition:")
            {
                
            }
            std::cout << "body vector = " << *it << std::endl;
        }
        // std::stringstream pair_stream(body_line);
    
    }

    // else if (content_type.find("text/plain") == 0 || content_type.empty())
    // {
    //     // raw string

    // }
    // else
    // {
    //     // multipart and else 
    // }
    // std::cout << "Content_length = " << content_length << std::endl;

    // check methods

    std::map<std::string,std::string>::iterator itt = body_map.begin();

    for (;itt != body_map.end();itt++)
    {
        std::cout << "key = " << itt->first << " | ";
        std::cout << "value = " << itt->second  << std::endl;
    }

    return ;
}

int main(int ac, char **av)
{
    int fd = open(av[1], O_RDONLY);
    read_from_fd(fd);
}