
#include "server.hpp"

Client::Client()
{

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

bool Client::handle_chunked_body(int client_fd) 
{
    (void)client_fd;

    while (!Hreq.body._body.empty()) {
        if (Hreq.body.reading_chunk_size) {
            size_t line_end = Hreq.body._body.find("\r\n");
            if (line_end == std::string::npos) {
                return true;
            }

            std::string size_str = Hreq.body._body.substr(0, line_end);
            Hreq.body.current_chunk_size = strtoul(size_str.c_str(), nullptr, 16);
            Hreq.body._body.erase(0, line_end + 2);
            Hreq.body.reading_chunk_size = false;

            if (Hreq.body.current_chunk_size == 0) {
                // Check for trailers (optional headers after last chunk)
                if (Hreq.body._body.size() >= 2) {
                    if (Hreq.body._body.substr(0, 2) == "\r\n") {
                        // No trailers, just empty line
                        Hreq.body._body.erase(0, 2);
                    } else {
                        // Has trailers - consume until empty line
                        size_t trailers_end = Hreq.body._body.find("\r\n\r\n");
                        if (trailers_end != std::string::npos) {
                            Hreq.body._body.erase(0, trailers_end + 4);
                        } else {
                            return true; // Need more data to complete trailers
                        }
                    }
                }
                Hreq.body.is_done = true;
                return false; // Chunked transfer complete
            }
        }

        if (Hreq.body._body.size() < Hreq.body.current_chunk_size + 2) {
            return true; // Need more data
        }

        Hreq.body.data.append(Hreq.body._body.substr(0, Hreq.body.current_chunk_size));
        Hreq.body._body.erase(0, Hreq.body.current_chunk_size + 2);
        Hreq.body.reading_chunk_size = true;
    }
    return true;
}



bool Client::read_from_fd(int client_fd)
{
    char recv_buffer[300];

    ssize_t n = read(client_fd, recv_buffer, sizeof(recv_buffer));
    std::cout << "read *from fd " << n << std::endl;

    if (n <= 0)
        return false;
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
    if (!Hreq.header.parsed)
    {
        size_t header_end = read_buffer.find("\r\n\r\n");
        if (header_end == std::string::npos) 
        {
            return true; // Need more data 
        }
        //  Parse the request line
        size_t pos = read_buffer.find("\r\n");
        if (pos == std::string::npos)
            return -1; //bad requestclient_fd
        std::string request_line = read_buffer.substr(0, pos);
        std::istringstream iss(request_line);
        iss >> Hreq.method >> Hreq.uri >> Hreq.http_v;
        
        
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
            return -1;
        }
        Hreq.header.parsed = true;

        // check transfer encoding or content-length
        if (Hreq.map_header.find("Content-Length") != Hreq.map_header.end())
        {
            std::string str = Hreq.map_header["Content-Length"];
            Hreq.body.expected_size = std::atoi(str.c_str());
            // Hreq.body.expected_size = std::stoi(Hreq.header.map_header["Content-Length"]);
            if (Hreq.header.map_header.find("Content-Type")  != Hreq.map_header.end()) 
            {
                Hreq.content_type = Hreq.map_header["Content-Type"];
                Hreq.content_type = trim(Hreq.content_type);
            }
        } 
        else if (Hreq.map_header.find("Transfer-Encoding")  != Hreq.map_header.end() &&
                    Hreq.map_header["Transfer-Encoding"].find("chunked") != std::string::npos) 
        {
                Hreq.body.chunked = true;
        }
                
         // Move remaining data (body) to body buffer
        Hreq.body._body = read_buffer.substr(header_end + 4);
        read_buffer.clear();
    }

    // Parse body depend of chunked or expected_size
    if (Hreq.header.parsed) 
    {
        std::string raw_body;
        std::istringstream dd(Hreq.body._body);
        if (Hreq.body.chunked) 
        {
            // while (true) 
            // {
            //     if (Hreq.body.reading_chunk_size) 
            //     {
            //         size_t line_end = Hreq.body._body.find("\r\n");
            //         if (line_end == std::string::npos)
            //             return true ; // need more data 
            //         std::string size_str = Hreq.body._body.substr(0, line_end);
            //         Hreq.body.current_chunk_size = std::strtol(size_str.c_str(), nullptr, 16);
            //         Hreq.body._body.erase(0, line_end + 2);
            //         Hreq.body.reading_chunk_size = false;

            //         if (Hreq.body.current_chunk_size == 0) 
            //         {
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
            handle_chunked_body(client_fd);
            if (Hreq.body.is_done) 
            {
                std::ofstream out("/tmp/request_body.txt");
                out << Hreq.body.data;
                out.close();
                Hreq.body.reset();
            }
            return !Hreq.body.is_done;
        }
        else if (Hreq.body.expected_size > 0)
        {
            // size_t to_read = std::min(Hreq.body.expected_size - Hreq.body.data.size(), Hreq.body._body.size());
            // Hreq.body.append(Hreq.body._body.substr(0, to_read));
            // Hreq.body._body.erase(0, to_read);

            // size_t to_read = std::min(Hreq.body.expected_size - Hreq.body.data.size(), Hreq.buffer.size());
            // Hreq.buffer.append(Hreq.buffer.substr(0, to_read));
            // Hreq.buffer.erase(0, to_read);
            if (Hreq.content_type.find("application/json") == 0 || Hreq.content_type == "text/plain")
            {
                std::string file_path = "/tmp/req_client_fd_" + std::to_string(client_fd);
                std::ofstream outfile(file_path.c_str());
                if (outfile.is_open()) 
                {
                    outfile << Hreq.body._body;
                    outfile.close();
                }
            }
            else if (Hreq.content_type.find("application/x-www-form-urlencoded") == 0)
            {
                // std::cout << "urlencoded" << std::endl;
                size_t p = read_buffer.find("\r\n\r\n");
                if (p != std::string::npos)
                    Hreq.body._body += read_buffer.substr(p + 4);

                std::string body_line;
                std::getline(dd, Hreq.body._body);

                std::stringstream pair_stream(Hreq.body._body);
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
                // std::cout << "| " << Hreq.body._body << " |" << std::endl;
                
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

                size_t pos = Hreq.content_type.find("boundary=");
                if (pos == std::string::npos) 
                {
                    return -1;
                }
                boundary = "--" + Hreq.content_type.substr(pos + 9);
                // boundary = "--" + Hreq.content_type.substr(Hreq.content_type.find("=") + 1,  Hreq.content_type.back()) + "\r\n";
                std::vector<std::string> body_vector = spliting(Hreq.body._body, boundary);
                // std::cout << "content-type = " << Hreq.content_type << std::endl;
                // std::string boundary = "--" + Hreq.content_type.substr(Hreq.content_type.find("=") + 1,  Hreq.content_type.back()) + "\r\n";
                // std::string boundary = Hreq.content_type.substr(Hreq.content_type.find("=") + 1);
                // size_t pos = Hreq.content_type.find("boundary=");
                // if (pos != std::string::npos) 
                // {
                //     boundary = "--" + Hreq.content_type.substr(pos + 9);
                //     std::cout << "boundary = " << boundary << std::endl;
                //     // std::vector<std::string> body_vector = spliting(Hreq.buffer, boundary);
                // }
                // std::string boundary = "------WebKitFormBoundary7MA4YWxkTrZu0gW\r\n";
                // std::cout << "body = " << Hreq.buffer << std::endl;


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
                            }
                            //find /r/n/r/n 
                            size_t pos = part.find("\r\n\r\n");
                            if (pos != std::string::npos)
                            {
                                size_t end = part.find("\r\n");
                                part =  part.substr(pos + 4, end - 1);
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
                    }
                // std::stringstream pair_stream(body_line);
            
            }
            if (Hreq.body.data.size() == Hreq.body.expected_size) 
            {
                Hreq.body.chunk_done = true;
                request_received = true;
            }
        }
    }
    return !request_received;
}


void Client::test_chunked_parsing() {
    std::cout << "=== Testing Chunked Transfer Encoding Parser ===\n\n";

    std::vector<std::vector<std::string>> test_cases = {
        {"5\r\nhello\r\n0\r\n\r\n"},
        {"5\r\nhe", "llo\r\n0", "\r\n\r\n"},
        {"5\r\nhello\r\n", "6\r\n world\r\n", "0\r\n\r\n"},
        {"5\r\nhello\r\n", "0\r\n", "X-Test: value\r\n\r\n"}
    };

    for (size_t i = 0; i < test_cases.size(); i++) {
        std::cout << "--- Test Case " << i+1 << " ---\n";
        Client client;
        
        client.Hreq.header.parsed = true;
        client.Hreq.header.map_header["Transfer-Encoding"] = "chunked";
        client.Hreq.body.chunked = true;
        
        bool need_more_data = true;
        
        for (size_t chunk_num = 0; chunk_num < test_cases[i].size(); chunk_num++) {
            const std::string& chunk = test_cases[i][chunk_num];
            std::cout << "Feeding chunk " << chunk_num+1 << ": [" << chunk << "]\n";
            
            // Append to body buffer directly for testing
            client.Hreq.body._body += chunk;
            
            need_more_data = client.handle_chunked_body(0);
            
            std::cout << "  Current body: \"" << client.Hreq.body.data << "\"\n";
            std::cout << "  Remaining buffer: \"" << client.Hreq.body._body << "\"\n";
            std::cout << "  Need more: " << (need_more_data ? "true" : "false") << "\n";
            std::cout << "  Parse state: " 
                      << (client.Hreq.body.reading_chunk_size ? "reading_size" : "reading_data") 
                      << ", current_chunk_size=" << client.Hreq.body.current_chunk_size << "\n";
            
            if (!need_more_data && chunk_num != test_cases[i].size() - 1) {
                std::cout << "ERROR: Parser completed too early!\n";
                break;
            }
        }
        
        if (!need_more_data && client.Hreq.body.is_done) {
            std::cout << "SUCCESS: Properly parsed complete chunked message\n";
            std::cout << "Final body: \"" << client.Hreq.body.data << "\"\n";
        } else if (need_more_data) {
            std::cout << "ERROR: Parser didn't detect complete message\n";
        } else {
            std::cout << "ERROR: Parser completed but didn't set is_done\n";
        }
        
        std::cout << "\n";
    }
}

void cleanup_connection(Server& sock, int fd) {
    close(fd);
    delete sock.sock_map[fd];
    sock.sock_map.erase(fd);
    epoll_ctl(sock.epoll_fd, EPOLL_CTL_DEL, fd, nullptr);
}

int main() 
{
    Server sock(AF_INET, SOCK_STREAM, 0);
    while(1) 
    {
        size_t i = epoll_wait(sock.epoll_fd, sock.events, 1024, -1);
        
        for (int j = 0; j < i; j++) 
        {
            if (sock.events[j].data.fd == sock.server_fd) 
            {
                // Handle new connection
                sock.client_fd = accept(sock.server_fd, 
                                      (struct sockaddr*)&sock.address, 
                                      &sock.addrlen);
                if (sock.client_fd == -1) 
                {
                    continue; 
                }
                
                // Set socket to non-blocking
                int flags = fcntl(sock.client_fd, F_GETFL, 0);
                fcntl(sock.client_fd, F_SETFL, flags | O_NONBLOCK);
                
                Client *A = new Client();
                sock.sock_map[sock.client_fd] = A;
                
                sock.client_events.events = EPOLLIN | EPOLLET;  // Edge-triggered
                sock.client_events.data.fd = sock.client_fd;
                epoll_ctl(sock.epoll_fd, EPOLL_CTL_ADD, sock.client_fd, &sock.client_events);
            }
            else 
            {
                int fd = sock.events[j].data.fd;
                Client* A = sock.sock_map[fd];
                
                if (sock.events[j].events & EPOLLIN) 
                {
                    bool connection_ok = true;
                    
                    // For edge-triggered, we must read until EAGAIN/EWOULDBLOCK
                    while (connection_ok) 
                    {
                        bool more_data_expected = A->read_from_fd(fd);
                        
                        if (A->Hreq.body.is_done) 
                        {
                            // Request fully received, prepare response
                            A->read_from_fd(sock.client_fd);
                            // Modify to watch for EPOLLOUT
                            sock.client_events.events = EPOLLOUT | EPOLLET;
                            epoll_ctl(sock.epoll_fd, EPOLL_CTL_MOD, fd, &sock.client_events);
                            break;
                        }
                        
                        if (!more_data_expected) 
                        {
                            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                                break;  // No more data available right now
                            } else {
                                // Real error occurred
                                connection_ok = false;
                            }
                        }
                    }
                    
                    if (!connection_ok) {
                        cleanup_connection(sock, fd);
                    }
                }
                if (sock.events[j].events & EPOLLOUT) 
                {
                    // Handle response writing
                    bool write_complete = A->write_to_fd(fd);
                    
                    if (write_complete) 
                    {
                        if (A->Hreq.header.map_header["Connection"] == "close") 
                        {
                            cleanup_connection(sock, fd);
                        } 
                        else 
                        {
                            // Keep alive - switch back to reading
                            A->reset_for_next_request();  // Implement this in Client
                            sock.client_events.events = EPOLLIN | EPOLLET;
                            epoll_ctl(sock.epoll_fd, EPOLL_CTL_MOD, fd, &sock.client_events);
                        }
                    } else if (errno != EAGAIN && errno != EWOULDBLOCK) 
                    {
                        cleanup_connection(sock, fd);
                    }
                }
                if (sock.events[j].events & (EPOLLERR | EPOLLHUP)) 
                {
                    cleanup_connection(sock, fd);
                }
            }
        }
    }
    return 0;
}

