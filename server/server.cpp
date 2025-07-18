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

    int optval = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
    {
        perror("setsockopt");
        close(server_fd);
        exit(0);
    }
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

std::vector<std::string> spliting( std::string& s, std::string& delimiter) 
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
    char recv_buffer[1024];

    ssize_t n = read(client_fd, recv_buffer, 1024);
    std::cout << "read *from fd " << n << std::endl;

    if (n <= 0)
        return false;
     read_buffer += std::string(recv_buffer, n);
    // if (read_buffer.find("\r\n\r\n") == std::string::npos)
    //     return;

    // std::cout << read_buffer  << std::endl;
    std::cout << "read_buffer = " << read_buffer << std::endl;
    size_t pos = read_buffer.find("\r\n");
    if (pos == std::string::npos)
        return -1;
    std::cout << "1*" << std::endl;
    if (!Hreq.header.parsed)
    {
        //  Parse the request line
        std::string request_line = read_buffer.substr(0, pos);
        std::string headers = read_buffer.substr(pos + 2, read_buffer.find("\r\n\r\n") - (pos + 2));
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
            std::cout << "2*" << std::endl;
            return -1;
        }
        Hreq.header.parsed = true;
        // check transfer encoding or content-length
        if (Hreq.map_header.find("Content-Length") != Hreq.map_header.end())
        {
            std::cout << "3*" << std::endl;
            std::string str = Hreq.map_header["Content-Length"];
            Hreq.body.expected_size = std::atoi(str.c_str());
            // Hreq.body.expected_size = std::stoi(Hreq.header.map_header["Content-Length"]);
            if (Hreq.header.map_header.find("Content-Type")  != Hreq.map_header.end()) 
            {
                std::cout << "here" << std::endl;
                Hreq.content_type = Hreq.map_header["Content-Type"];
                Hreq.content_type = trim(Hreq.content_type);
            }
        } 
        else if (Hreq.map_header.find("Transfer-Encoding")  != Hreq.map_header.end() &&
                    Hreq.map_header["Transfer-Encoding"].find("chunked") != std::string::npos) 
        {
                Hreq.body.chunked = true;
        }
    }
    else 
    {
        std::cout << "else" << std::endl;
        return true;
    }
    std::cout << "expected_size = " <<  Hreq.body.expected_size << std::endl;
    // Parse body depend of chunked or expected_size
    if (Hreq.header.parsed) 
    {
        size_t p = read_buffer.find("\r\n\r\n");
        Hreq.buffer  = read_buffer.substr(p + 2, read_buffer.size() - 1);
        
        std::istringstream dd(Hreq.buffer);
        if (Hreq.body.chunked) 
        {
            // chunked parsing
            std::string str =  Hreq.map_header["Transfer-Encoding"];
            if (str.find("chunked") != std::string::npos)
            {
                size_t pos = 0;
                while (true)
                {
                    size_t size_end = Hreq.buffer.find("\r\n", pos);
                    if (size_end == std::string::npos)
                        return -1;

                    std::string size_str = Hreq.buffer.substr(pos, size_end - pos);
                    int chunk_size = std::atoi(size_str.c_str());

                    pos = size_end + 2;
                   if (chunk_size == 0 && Hreq.buffer.find("\r\n\r\n", pos) != std::string::npos)
                        break;
                    if (Hreq.buffer.size() < pos + chunk_size + 2)
                        throw std::out_of_range("Incomplete chunk data or missing CRLF");

                    std::string chunk_data = Hreq.buffer.substr(pos, chunk_size);

                    try 
                    {
                        if (Hreq.buffer.substr(pos + chunk_size, 2) != "\r\n")
                            throw std::out_of_range("Missing CRLF after chunk data");
                    }
                    catch (std::out_of_range &e)
                    {
                        std::cout << e.what() << std::endl;
                        return -1;
                    }
                    // chunks.push_back(std::make_pair(chunk_size, chunk_data));
                    file_path = "/tmp/req_client_fd_" + std::to_string(client_fd);
                    std::ofstream outfile(file_path.c_str());
                    if (outfile.is_open()) 
                    {
                        outfile  << Hreq.buffer;
                        outfile.close();
                    }
                    pos += chunk_size + 2;
                }
            }
        }
        else if (Hreq.body.expected_size > 0) 
        {
            
            size_t to_read = std::min(Hreq.body.expected_size - Hreq.body.data.size(), Hreq.buffer.size());
            Hreq.buffer.append(Hreq.buffer.substr(0, to_read));
            Hreq.buffer.erase(0, to_read);
            std::cout << "body = " << Hreq.buffer << std::endl;
            std::cout << "TEST" << std::endl;
            std::cout << "content_type" <<  Hreq.content_type << std::endl;
            if (Hreq.content_type.find("application/json") == 0 || Hreq.content_type == "text/plain")
            {
                std::cout << "app/json" << std::endl;
                std::string file_path = "/tmp/req_client_fd_" + std::to_string(client_fd);
                std::ofstream outfile(file_path.c_str());
                if (outfile.is_open()) 
                {
                    outfile << Hreq.buffer;
                    outfile.close();
                }
            }
            else if (Hreq.content_type.find("application/x-www-form-urlencoded") == 0)
            {
                // std::cout << "urlencoded" << std::endl;
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
                        Hreq.body_map.insert(std::make_pair(key, value));
                    }
                }
            }
            else if (Hreq.content_type.find("multipart/form-data") == 0)
            {
                std::cout << "multipart" << std::endl;
                std::cout << "content-type = " << Hreq.content_type << std::endl;
                // std::string boundary = "--" + Hreq.content_type.substr(Hreq.content_type.find("=") + 1,  Hreq.content_type.back()) + "\r\n";
                std::string boundary = Hreq.content_type.substr(Hreq.content_type.find("=") + 1);
                // std::string boundary = "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n";
                std::cout << "boundary = " << boundary << std::endl;
                // std::cout << "body = " << Hreq.buffer << std::endl;

                std::vector<std::string> body_vector = spliting(Hreq.buffer, boundary);

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
                                file_path = "/tmp/req_client_fd_" + std::to_string(client_fd);
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
            if (Hreq.body.data.size() == Hreq.body.expected_size) 
            {
                Hreq.body.complete = true;
                request_received = true;
            }
        }
    }
    // std::map<std::string,std::string>::iterator itt = Hreq.body_map.begin();

    // for (;itt != Hreq.body_map.end();itt++)
    // {
    //     std::cout << "key = " << itt->first << " | ";
    //     std::cout << "value = " << itt->second  << std::endl;
    // }

    return !request_received;
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
