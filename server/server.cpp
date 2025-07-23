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

    isGET = false;
    isPOST = false;
    isDELETE = false;
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

void Client::reset_for_next_request() 
{
    Hreq.header.parsed = false;
    Hreq.header.method.clear();
    Hreq.header.uri.clear();
    Hreq.header.http_v.clear();
    Hreq.header.map_header.clear();
    Hreq.header.content_type.clear();
    request_received = false;


    Hreq.body.raw_body.clear();
    Hreq.body.data.clear();
    Hreq.body._body.clear();
    // Hreq.body._body.clear();
    Hreq.body.expected_size = 0;
    Hreq.body.chunked = false;
    Hreq.body.chunk_done = false;
    Hreq.body.is_done = false;
    Hreq.body.current_chunk_size = 0;
    Hreq.body.reading_chunk_size = true;
    read_buffer.clear();
    Hreq.buffer.clear();
    write_buffer.clear();
    response_sent = false;
    file_path.clear();
    Hreq.body_map.clear();
    // request_received = false;
}
void Client::handle_chunked_body() 
{
    while (true) {
        if (Hreq.body.reading_chunk_size) 
        {
            size_t line_end = Hreq.body._body.find("\r\n");
            if (line_end == std::string::npos) 
            {
                return;
            }

            std::string size_str = Hreq.body._body.substr(0, line_end);
            Hreq.body._body.erase(0, line_end + 2); // remove size line

            Hreq.body.current_chunk_size = std::strtol(size_str.c_str(), nullptr, 16);

            if (Hreq.body.current_chunk_size == 0) 
            {
                Hreq.body.is_done = true;
                return;
            }

            Hreq.body.reading_chunk_size = false;
        }

        if (Hreq.body._body.size() < Hreq.body.current_chunk_size + 2) 
        {
            return;
        }

        Hreq.body.data += Hreq.body._body.substr(0, Hreq.body.current_chunk_size);
        Hreq.body._body.erase(0, Hreq.body.current_chunk_size + 2);
        Hreq.body.reading_chunk_size = true;
    }
}

void Client::getMethod()
{

}

void Client::deleteMethod()
{

}

RequestParseStatus Client::read_from_fd(int client_fd)
{
    char recv_buffer[1024];

    ssize_t n = recv(client_fd, recv_buffer, sizeof(recv_buffer),0);
    std::cout << "read *from fd " << n << std::endl;
    
    if (n == 0) {
        std::cout << "Client closed connection" << std::endl;;
        // return 0;
        return PARSE_CONNECTION_CLOSED;
    }
    if (n < 0) {
    // if (errno == EAGAIN || errno == EWOULDBLOCK) {
    //         std::cout << "DEBUG: EAGAIN — try again later\n";
    //         return 0;
    //     }
        std::cout << "****read error: " << "\n";
       return PARSE_INTERNAL_ERROR; // Only real errors
    }

    //  read_buffer += std::string(recv_buffer, n);
    // if (read_buffer.find("\r\n\r\n") == std::string::npos)
    //     return;
    read_buffer.append(recv_buffer, n);
    // Append to raw buffer
    // std::string new_data(recv_buffer, n);
    // Hreq.body.append(new_data);

    // read_buffer =
    //     "POST /upload HTTP/1.1\r\n"
    //     "Host: example.com\r\n"
    //     "User-Agent: curl/7.68.0\r\n"
    //     "Accept: */*\r\n"
    //     "Transfer-Encoding: chunked\r\n"
    //     "Content-Type: text/plain\r\n"
    //     "\r\n"
    //     "5\r\n"
    //     "Hello\r\n"
    //     "6\r\n"
    //     " World\r\n"
    //     "0\r\n"
    //     "\r\n";
    std::string boundary;

    // size_t pos = read_buffer.find("\r\n");
    // if (pos == std::string::npos)
    //     return -1;

    // std::string request_line = read_buffer.substr(0, pos);
    // std::string headers = read_buffer.substr(pos + 2, read_buffer.find("\r\n\r\n") - (pos + 2));
    // size_t p = read_buffer.find("\r\n\r\n");
    // std::cout << "p = "  <<  p << std::endl;

    // Hreq.body._body = read_buffer.substr(p + 4, read_buffer.size() - 1);
    // std::cout << "read_buffer = " << read_buffer << std::endl;
    // std::cout <<  "BODY = " << Hreq.body._body << std::endl;

    // Hreq.buffer = read_buffer.substr(p + 4, read_buffer.size() - 1);
    std::cout << "TEST" << std::endl;
    if (!Hreq.header.parsed)
    {
        std::cout << "read_buffer = " << read_buffer << std::endl;
        size_t header_end = read_buffer.find("\r\n\r\n");
        if (header_end == std::string::npos) 
        {
            return PARSE_INCOMPLETE; // Need more data 
        }
        //  Parse the request line
        size_t pos = read_buffer.find("\r\n");
        if (pos == std::string::npos)
            return PARSE_BAD_REQUEST;; //bad request
        std::cout << "TEST3" << std::endl;
        std::string request_line = read_buffer.substr(0, pos);
        std::istringstream iss(request_line);
        iss >> Hreq.method >> Hreq.uri >> Hreq.http_v;

        if (Hreq.uri.length() > 4096)
        {
            return REQUEST_URI_TOO_LONG;
        }
        //parse headers 
        std::string headers = read_buffer.substr(pos + 2, read_buffer.find("\r\n\r\n") - (pos + 2));
        std::string header_line;
        std::istringstream ss(headers);
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
        if (!check_methods())
        {
            std::cout << "Error Method" << std::endl;
            std::cout << "2*" << std::endl;
            return PARSE_NOT_IMPLEMENTED;
        }
        Hreq.header.parsed = true;


        std::cout << "TEST4" << std::endl;
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
        if (!Hreq.body.chunked && Hreq.body.expected_size == 0)
        {
            request_received = true;
            return PARSE_OK;
        }
        Hreq.body._body = read_buffer.substr(header_end + 4);
        // read_buffer.clear(); // ghda ngul lik fin <3
    }

    std::cout << "expected_size = " <<  Hreq.body.expected_size << std::endl;
    
    if (Hreq.header.parsed) 
    {
        std::string raw_body;
        std::istringstream dd(Hreq.body._body);
        if (Hreq.body.chunked) 
        {
            handle_chunked_body();
            if (Hreq.body.is_done) 
            {
                std::ofstream out("/tmp/request_body.txt");
                out << Hreq.body.data;
                out.close();
                Hreq.body.reset();
                request_received = true;
                return PARSE_OK;
            }
            return PARSE_INCOMPLETE;
        }
        // if (Hreq.body.chunked) 
        // {
            // while (true) 
            // {
            //     if (Hreq.body.reading_chunk_size) 
            //     {
            //         size_t line_end = Hreq.body._body.find("\r\n");
            //         if (line_end == std::string::npos)
            //             return true ; // need more data 
            //         std::string size_str = Hreq.body._body.substr(0, line_end);
            //         Hreq.body.current_chunk_size = std::strtol(size_str.c_str(), nullptr, 16);
            //         std::cout << "current_chunk_size = 0 " << Hreq.body.current_chunk_size << std::endl;
            //         Hreq.body._body.erase(0, line_end + 2);
            //         std::cout  << "after erase = " <<  Hreq.body._body << std::endl;
            //         Hreq.body.reading_chunk_size = false;

            //         if (Hreq.body.current_chunk_size == 0) 
            //         {
            //             std::cout << "true" << std::endl;
            //             Hreq.body.is_done = true;
            //             request_received = true;
            //             break;
            //         }
            //     }
            //     if (Hreq.body._body.size() < Hreq.body.current_chunk_size + 2)
            //         break; // wait for more data

            //     Hreq.body.data += Hreq.body._body.substr(0, Hreq.body.current_chunk_size);
            //     Hreq.body._body.erase(0, Hreq.body.current_chunk_size + 2);
            //     Hreq.body.reading_chunk_size = true;
            //     // std::string chunk_data = Hreq.body._body.substr(0, Hreq.body.current_chunk_size);
            //     // Hreq.body.data += chunk_data;
            //     // Hreq.body._body.erase(0, Hreq.body.current_chunk_size + 2); // +2 to skip trailing \r\n
            //     // std::cout << "body.data = "<<  Hreq.body.data << std::endl;
            //     Hreq.body.reading_chunk_size = true;
            // }
            // handle_chunked_body();
            // if (Hreq.body.is_done) 
            // {
            //     std::ofstream out("/tmp/request_body.txt");
            //     out << Hreq.body.data;
            //     out.close();
            //     Hreq.body.reset();
            // }
        //     return !Hreq.body.is_done;
        // }
        else if (Hreq.body.expected_size > 0)
        {
            std::cout << "BUFFER = " << read_buffer << std::endl;
            size_t p = read_buffer.find("\r\n\r\n");

            std::string body = read_buffer.substr(p + 4, read_buffer.size() - 1);
            // size_t to_read = std::min(Hreq.body.expected_size - Hreq.body.data.size(), Hreq.body._body.size());
            // Hreq.body.append(Hreq.body._body.substr(0, to_read));
            // Hreq.body._body.erase(0, to_read);

            // size_t to_read = std::min(Hreq.body.expected_size - Hreq.body.data.size(), Hreq.buffer.size());
            // Hreq.buffer.append(Hreq.buffer.substr(0, to_read));
            // Hreq.buffer.erase(0, to_read);
            
            std::cout << "BODY = " << Hreq.body._body << std::endl;
            std::cout << "Content_Type = " <<  Hreq.content_type << std::endl;
            if (Hreq.content_type.find("application/json") == 0 || Hreq.content_type == "text/plain")
            {
                std::cout << "app/json" << std::endl;
                std::string file_path = "/tmp/req_client_fd_" + std::to_string(client_fd);
                std::ofstream outfile(file_path.c_str());
                if (outfile.is_open()) 
                {
                    outfile << Hreq.body._body;
                    outfile.close();
                    // return true;
                }
                Hreq.body.chunk_done = true;
                request_received = true;
            }
            else if (Hreq.content_type.find("application/x-www-form-urlencoded") == 0)
            {
                std::cout << "urlencoded" << std::endl;

                // size_t p = read_buffer.find("\r\n\r\n");
                // if (p != std::string::npos)
                    // Hreq.body._body += read_buffer.substr(p + 4);
                std::cout << "urlencoded1" << std::endl;

                std::string body_line;
                std::getline(dd, Hreq.body._body);

                std::stringstream pair_stream(Hreq.body._body);
                std::string pair;

                while (std::getline(pair_stream, pair, '&'))
                {
                    std::cout << "urlencoded3" << std::endl;
                    std::cout << "cout = " <<  pair << std::endl;
                    size_t equal_pos = pair.find('=');
                    if (equal_pos != std::string::npos)
                    {
                        std::cout << "urlencoded4" << std::endl;
                        std::string key = pair.substr(0, equal_pos);
                        std::string value = pair.substr(equal_pos + 1);
                        std::cout << "key = " << key << std::endl;
                        std::cout << "value = " << value << std::endl;

                        Hreq.body_map.insert(std::make_pair(key, value));
                    }
                }
                // std::map<std::string, std::string>::iterator ittt = Hreq.body_map.begin();

                // for(;ittt  != Hreq.body_map.end(); ittt++)
                // {
                //     std::cout << *ittt
                // }
                // if (Hreq.body_map )
                Hreq.body.chunk_done = true;
                request_received = true;
            }
            else if (Hreq.content_type.find("multipart/form-data") == 0)
            {
                // std::cout << "| " << Hreq.body._body << " |" << std::endl;
                
                // std::cout << "THE BODY IS = " <<  Hreq.body._body << std::endl;
                // Hreq.body._body = 
                // "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
                // "Content-Disposition: form-data; name=\"firstName\"\r\n"
                // "\r\n"
                // "Brian\r\n"
                // "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n"
                // "Content-Disposition: form-data; name=\"file\"; filename=\"hello.txt\"\r\n"
                // "Content-Type: text/plain\r\n"
                // "\r\n"
                // "Hello, this is the content of the file.\r\n"
                // "------WebKitFormBoundary7MA4YWxkTrZu0gW--\r\n";

                // std::cout << "content type = " << Hreq.content_type << std::endl;
                size_t pos = Hreq.content_type.find("boundary=");
                if (pos == std::string::npos) 
                {
                    return PARSE_BAD_REQUEST;
                }
                boundary = "--" + Hreq.content_type.substr(pos + 9);
                // std::cout << "boundary = " <<  boundary << std::endl;
                // boundary = "--" + Hreq.content_type.substr(Hreq.content_type.find("=") + 1,  Hreq.content_type.back()) + "\r\n";
                std::vector<std::string> body_vector = spliting(Hreq.body._body, boundary);
                std::cout << "multipart" << std::endl;

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
                            std::cout << "part = " << part << std::endl;
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
                Hreq.body.chunk_done = true;
                request_received = true;
            }
        }
    }
    
    if (Hreq.method == "GET")
    {
        isGET = true;
        getMethod();
        // Hreq.uri;
        // Hreq.http_v;
        // Hreq.method;
        // status 
    }
    if (request_received)
        return PARSE_OK; 
    else
        return PARSE_INCOMPLETE;
}


bool Client::write_to_fd(int fd)
{
    std::cout << ">> Attempting to write to fd\n";

    if (!response_ready || response_buffer.empty()) 
    {
        std::cout << ">> Not ready to write yet\n";
        return true; // Nothing to write yet
    }

    ssize_t sent = send(fd, response_buffer.c_str(), response_buffer.length(), 0);
    std::cout << ">> Sent bytes: " << sent << "\n";

    if (sent <= 0)
    {
        std::cerr << ">> Failed to send response or client closed connection\n";
        return false;
    }

    if (static_cast<size_t>(sent) == response_buffer.length()) {
        std::cout << ">> Response sent completely\n";
        response_buffer.clear();
        response_ready = false;
        return true;
    }

    response_buffer = response_buffer.substr(sent);
    std::cout << ">> Partial send. Remaining: " << response_buffer.length() << "\n";
    return false;
}



// bool Client::write_to_fd(int fd)
// {
//     const std::string response =
//         "HTTP/1.1 200 OK\r\n"
//         "Content-Length: 13\r\n"
//         "Content-Type: text/plain\r\n"
//         "\r\n"
//         "Hello, world!";

//     ssize_t sent = send(fd, response.c_str(), response.length(), 0);
//     if (sent <= 0)
//     {
//         std::cerr << "Failed to send response or client closed connection\n";
//         return false;
//     }

//     std::cout << "Response sent. Closing connection.\n";
//     return false; 
// }
