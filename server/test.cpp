#include <algorithm>
#include <cstddef>
#include <cstring>
#include <iostream>
#include <sstream>
#include "server.hpp"
#include <fcntl.h>
#include <stdexcept>
#include <string>
#include <sys/types.h>
#include <unordered_map>
#include <utility>

#include <fstream>
#include <vector>

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

    // content-length | transfer-encoding : ...chunked
    /*
        size CRLF
        DATA CRLF
    */
        //  Json
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
    
    //application/x-www-form-urlencoded
       
    //MULTIPART
    // read_buffer =
    //     "POST /upload HTTP/1.1\r\n"
    //     "Host: example.com\r\n"
    //     "User-Agent: curl/7.68.0\r\n"
    //     "Accept: */*\r\n"
    //     "Content-Type: multipart/form-data; boundary=----WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
    //     "Content-Length: 314\r\n"
    //     "\r\n"
    //     "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
    //     "Content-Disposition: form-data; name=\"firstName\"\r\n"
    //     "\r\n"
    //     "Brian\r\n"
    //     "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
    //     "Content-Disposition: form-data; name=\"file\"; filename=\"hello.txt\"\r\n"
    //     "Content-Type: text/plain\r\n"
    //     "\r\n"
    //     "Hello, this is the content of the file.\r\n"
    //     "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n";
        
        // TRANSFER ENCODING : CHUNCKED 
        read_buffer =
        "POST /upload HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "User-Agent: curl/7.68.0\r\n"
        "Accept: */*\r\n"
        "Transfer-Encoding: chunked\r\n"
        "Content-Type: text/plain\r\n"
        "\r\n"
        "7\r\n"
        "Mozilla\r\n"
        "9\r\n"
        "Developer\r\n"
        "0\r\n"
        "Network\r\n"
        "0\r\n"
        "\r\n";

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
            // std::cout << "Key :" <<  key << std::endl;
            // std::cout << "value :"  << value << std::endl;
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
    std::vector<std::pair<int, std::string> > chunks;

    if (method == "POST")
    {
        if (map_header.find("Content-Length") != map_header.end()) 
        {
            std::string str = map_header["Content-Length"];
            content_length = std::atoi(str.c_str());
        }
        
        else if (map_header.find("Transfer-Encoding") != map_header.end())
        {
            std::string str = map_header["Transfer-Encoding"];
            if (str.find("chunked") != std::string::npos)
            {
                std::cout << "Chunked transfer detected.\n";

                size_t pos = 0;
                while (true)
                {
                    size_t size_end = body.find("\r\n", pos);
                    if (size_end == std::string::npos)
                        return ;

                    std::string size_str = body.substr(pos, size_end - pos);
                    int chunk_size = std::atoi(size_str.c_str());

                    pos = size_end + 2;
                    if (chunk_size == 0 && body.find("\r\n\r\n", pos))
                        break ;
                    if (body.size() < pos + chunk_size + 2)
                        throw std::out_of_range("Incomplete chunk data or missing CRLF");

                    std::string chunk_data = body.substr(pos, chunk_size);

                    try 
                    {
                        if (body.substr(pos + chunk_size, 2) != "\r\n")
                            throw std::out_of_range("Missing CRLF after chunk data");
                    }
                    catch (std::out_of_range &e)
                    {
                        std::cout << e.what() << std::endl;
                    }

                    chunks.push_back(std::make_pair(chunk_size, chunk_data));

                    pos += chunk_size + 2;
                }
                
                for (size_t i = 0; i < chunks.size(); ++i)
                {
                    std::cout << "Chunk " << i << ": size=" << chunks[i].first
                            << ", data=\"" << chunks[i].second << "\"\n";
                }
            }
        }

        // check Transfer-Encoding
    }

    //




    //
    std::cout << "content_length = " << content_length << std::endl;

    if (content_type.find("application/json") == 0 || content_type == "text/plain")
    {
        std::string file_path = "/tmp/req_client_fd_" + std::to_string(client_fd);
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

        std::string filename;
        std::string name;
        std::string content;
        std::vector<std::string>::iterator it = body_vector.begin();
        for(;it != body_vector.end(); it++)
        {
            if (*it == "")
                continue;
            std::string part = *it;
            size_t disp_pos = part.find("Content-Disposition:");
            if (disp_pos != std::string::npos)
            {
                size_t name_pos = part.find("filename=\"", disp_pos);
                if (name_pos != std::string::npos)
                {
                    name_pos += 10;
                    size_t end_quote = part.find("\"", name_pos);
                    if (end_quote != std::string::npos)
                    {
                        filename = part.substr(name_pos, end_quote - name_pos);
                        std::cout << "filename = " << filename << std::endl;
                    }
                    //find /r/n/r/n 
                    size_t pos = part.find("\r\n\r\n");
                    if (pos != std::string::npos)
                    {
                        size_t end = part.find("\r\n");
                        part =  part.substr(pos + 4, end - 1);
                        std::cout << "Part = " << part;
                    }
                    std::string file_path = "/tmp/" + filename;
                    std::ofstream outfile(file_path.c_str());
                    if (outfile.is_open()) 
                    {
                        outfile << part;
                        outfile.close();
                    }
                }
                else 
                {
                    size_t name_pos = part.find("name=\"", disp_pos);
                    if (name_pos != std::string::npos)
                    {
                        name_pos += 6;
                        size_t end_quote = part.find("\"", name_pos);
                        if (end_quote != std::string::npos)
                        {
                            name = part.substr(name_pos, end_quote - name_pos);
                            std::cout << "name = " << name << std::endl;
                        }
                    }
                    //
                    size_t pos = part.find("\r\n\r\n");
                    if (pos != std::string::npos)
                    {
                        size_t end = part.find("\r\n");
                        content = part.substr(pos + 4, end - 1);
                        std::string file_path = "/tmp/req_client_fd_" + std::to_string(client_fd);
                        std::ofstream outfile(file_path.c_str());
                        if (outfile.is_open()) 
                        {
                            outfile << content;
                            outfile.close();
                        }
                        std::cout << "content = " << content;
                    }
                    // store the content in map
                    // std::map<std::string,std::string> multipart_body;
                    // multipart_body.insert(std::make_pair(name, content));
                }
            }
            // find /r/n/r/n
            // size_t pos = part.find("\r\n\r\n");
            // if (pos != std::string::npos)
            // {
            //     size_t end = part.find("\r\n");
            //     part = part.substr(pos + 4, end - 1);
            //     std::cout << "Part = " << part;
            // }
            ///-------------
            std::cout << "body vector = " << *it << std::endl;
            }
        // std::stringstream pair_stream(body_line);
    
    }

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