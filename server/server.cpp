#include "server.hpp"


Server::Server(const ServerCo& conf) : config(conf)
{
    // domain = AF_INET;
    // type = SOCK_STREAM;
    // protocol = 0;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0)
        throw std::runtime_error("Failed to create socket");

    address.sin_family = AF_INET;
    address.sin_port = htons(conf.listen);
    // address.sin_addr.s_addr = inet_addr(conf.host.c_str());
    address.sin_addr.s_addr = htonl(INADDR_ANY);

    int optval = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
    {
        perror("setsockopt");
        close(server_fd);
        exit(0);
    }
    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)))
         throw std::runtime_error("Bind failed");
    
    if (listen(server_fd, SOMAXCONN) < 0)
        throw std::runtime_error("Listen failed");

    epoll_fd = epoll_create(1);
    // server_event.events = EPOLLIN;
    server_event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    server_event.data.fd = server_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &server_event);
    addrlen = sizeof(address);
     std::cout << "Listening on " << conf.host << ":" << conf.listen << std::endl;
}

Client::Client()
{
    send_offset = 0;
    cgi_bin = false;
    request_received = false;
    response_sent = false;
    is_cgi = false;
    isGET = false;
    isPOST = false;
    isDELETE = false;
}

void Body::append(const std::string& new_data)
{
    data += new_data;
}

void Body::reset() 
{
    data.clear();
    raw_body.clear();
    expected_size = 0;
    chunked = false;
    is_done = false;
    current_chunk_size = 0;
    reading_chunk_size = true;
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

// bool Client::check_methods()
// {

// }

void Client::reset_for_next_request() 
{
    send_offset = 0;
    std::cout << "@@@@ Resetting client state for next request\n";
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


void Client::handle_chunked_body(size_t max_body_size) 
{
    std::cout << "handle_chunked_body" << std::endl;
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
        if (Hreq.body.data.size() > max_body_size) 
        {
            throw std::runtime_error("Payload too large");
        }
        Hreq.body._body.erase(0, Hreq.body.current_chunk_size + 2);
        Hreq.body.reading_chunk_size = true;
    }
}

std::string Client::ft_content_type(const std::string& full_path)
{
    if (full_path.find(".html") != std::string::npos)
        return "text/html";
    else if (full_path.find(".css") != std::string::npos)
        return "text/css";
    else if (full_path.find(".js") != std::string::npos)
        return "application/javascript";
    else if (full_path.find(".png") != std::string::npos)
        return "image/png";
    else if (full_path.find(".jpg") != std::string::npos || full_path.find(".jpeg") != std::string::npos)
        return "image/jpeg";
    else if (full_path.find(".gif") != std::string::npos)
        return "image/gif";
    else if (full_path.find(".svg") != std::string::npos)
        return "image/svg+xml";
    else if (full_path.find(".ico") != std::string::npos)
        return "image/x-icon";
    else if (full_path.find(".json") != std::string::npos)
        return "application/json";
    else if (full_path.find(".txt") != std::string::npos)
        return "text/plain";
    else if (full_path.find(".xml") != std::string::npos)
        return "application/xml";
    else if (full_path.find(".pdf") != std::string::npos)
        return "application/pdf";
    else if (full_path.find(".mp4") != std::string::npos)
        return "video/mp4";
    else if (full_path.find(".mp3") != std::string::npos)
        return "audio/mpeg";
    else if (full_path.find(".woff") != std::string::npos)
        return "font/woff";
    else if (full_path.find(".woff2") != std::string::npos)
        return "font/woff2";
    else if (full_path.find(".ttf") != std::string::npos)
        return "font/ttf";
    else if (full_path.find(".otf") != std::string::npos)
        return "font/otf";
    else if (full_path.find(".eot") != std::string::npos)
        return "application/vnd.ms-fontobject";
    else if (full_path.find(".zip") != std::string::npos)
        return "application/zip";
    else if (full_path.find(".tar") != std::string::npos)
        return "application/x-tar";
    else if (full_path.find(".gz") != std::string::npos)
        return "application/gzip";
    else if (full_path.find(".rar") != std::string::npos)
        return "application/x-rar-compressed";
    else if (full_path.find(".csv") != std::string::npos)
        return "text/csv";
    else if (full_path.find(".md") != std::string::npos)
        return "text/markdown";
    else if (full_path.find(".py") != std::string::npos)
        return "application/x-python-code";
    else if (full_path.find(".php") != std::string::npos)
        return "application/x-php";   
    else if (full_path.find(".pl") != std::string::npos)
        return "text/html";  
    else if (full_path.find(".yaml") != std::string::npos || full_path.find(".yml") != std::string::npos)
        return "application/x-yaml";
    else
        return "application/octet-stream"; // Default type for unknown extensions
}

std::string generate_temp_file_path()
{
    char filename[] = "/tmp/body_XXXXXX";
    int fd = mkstemp(filename);
    if (fd == -1)
    {
        std::cerr << "mkstemp failed: " << strerror(errno) << std::endl;
        return "";
    }
    close(fd);
    return std::string(filename);
}



RequestParseStatus Client::read_from_fd(int client_fd, long long max_body_size)
{
    // std::stringstream response;
    // std::cout << "herere" << std::endl;
    // response << "HTTP/1.1 200 OK\r\n";
    // // response << headers << "\r\n\r\n";
    // // response << "Content-Type: video/mp4\r\n";
    // response << "Content-Type: text/html\r\n";
    // // std::cout << "script_file = "  << 
    // // std::cout << ft_content_type(script_file)  << std::endl;
    // // response << "Content-Type: " << ft_content_type(script_file) << "\r\n";
    // response << "Content-Length: " << cgi_output.size() << "\r\n";
    // response << "\r\n";
    // response << cgi_output;
    // response_buffer = response.str();

    char recv_buffer[8192];
    // 
    this->client_fd = client_fd;
    while (true)
    {
        ssize_t n = recv(client_fd, recv_buffer, sizeof(recv_buffer),0);
        if (n < 0)
        {
            break;
            // return PARSE_CONNECTION_CLOSED;
        }
        if (n == 0)
            return PARSE_CONNECTION_CLOSED;

        read_buffer.append(recv_buffer, n);
        memset(recv_buffer, 0, sizeof(recv_buffer));
    }

    std::string boundary;
    if (!Hreq.header.parsed)
    {
        std::cout << "read_buffer = " << read_buffer << std::endl;
        size_t header_end = read_buffer.find("\r\n\r\n");
        std::cerr << "[DEBUG] read_buffer size: " << read_buffer.size() << std::endl;

        if (header_end == std::string::npos) 
        {
            return PARSE_INCOMPLETE;
        }
        //  Parse the request line
        size_t pos = read_buffer.find("\r\n");
        if (pos == std::string::npos)
            return PARSE_BAD_REQUEST;; //bad request
        std::string request_line = read_buffer.substr(0, pos);
        std::istringstream iss(request_line);
        iss >> Hreq.method >> Hreq.uri >> Hreq.http_v;
        std::cerr << "[DEBUG] After parsing method: '" << Hreq.method << "'" << std::endl;
        if (Hreq.method == "GET")
        {
            isGET = true;
        }
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
        // if (!check_methods())
        // {
        //     std::cout << "Error Method" << std::endl;
        //     return REQUEST_METHOD_NOT_ALLOWED;
        // }
        Hreq.header.parsed = true;
        if (is_cgi_request())
        {
            std::cout << "IS CGI " << std::endl;
            is_cgi = true;
        }
        std::cout << "HERE" << std::endl;
        
        // check transfer encoding or content-length
        if (Hreq.method == "POST")
        {
            if (Hreq.map_header.find("Content-Length") != Hreq.map_header.end())
            {
                std::string str = Hreq.map_header["Content-Length"];
                Hreq.body.expected_size = std::atoi(str.c_str());
                
                // check max body size
                std::cout << "max_body_size = " << max_body_size << std::endl;
                std::cout << "expected_size = " << Hreq.body.expected_size << std::endl;
                if (Hreq.body.expected_size > max_body_size)
                    return PAYLOAD_TOO_LARGE;
                // Hreq.body.expected_size = std::stoi(Hreq.header.map_header["Content-Length"]);
                if (Hreq.header.map_header.find("Content-Type")  != Hreq.map_header.end()) 
                {
                    std::cout << "here" << std::endl;
                        Hreq.content_type = Hreq.map_header["Content-Type"];
                        Hreq.content_type = trim(Hreq.content_type);
                }
                else
                {
                    return PARSE_BAD_REQUEST;
                }
            }
            else {
                return PARSE_BAD_REQUEST;
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
        // Hreq.body._body = read_buffer.substr(header_end + 4);
        Hreq.body._body.append(read_buffer.substr(header_end + 4));
        if (Hreq.body._body.size() > max_body_size) 
        {
            return PAYLOAD_TOO_LARGE;
        }
        // read_buffer.clear(); // ghda ngul lik fin <3
    }
    std::cout << "HERE2" << std::endl;

    std::cout << "expected_size = " <<  Hreq.body.expected_size << std::endl;
    
    std::cout << Hreq.header.parsed << " <---- header parsed" <<  std::endl;
    if (Hreq.header.parsed) 
    {
        std::string raw_body;
        std::istringstream dd(Hreq.body._body);
        if (Hreq.body.chunked) 
        {
            try 
            {
                handle_chunked_body(max_body_size);
            }
            catch (const std::exception &e)
            {
                return PAYLOAD_TOO_LARGE;
            }
            if (Hreq.body.is_done) 
            {
                if (Hreq.method == "POST")
                {
                    std::string file_path = generate_temp_file_path();
                    std::cout << "file_path = " <<  file_path << std::endl;
                    std::ofstream outfile(file_path.c_str());
                    if (!outfile.is_open()) 
                    {
                        std::cerr << "Failed to open file: " << file_path << std::endl;
                    } 
                    else 
                    {
                        outfile << Hreq.body.data;
                        outfile.close();
                    }
                    Hreq.body.reset();
                    request_received = true;
                }
                return PARSE_OK;
            }
            return PARSE_INCOMPLETE;
        }
        else if (Hreq.body.expected_size > 0)
        {
            std::cout << "BUFFER = " << read_buffer << std::endl;
            std::cout << "BODY = " << Hreq.body._body << std::endl;
            std::cout << "Content_Type = " <<  Hreq.content_type << std::endl;
            if (Hreq.content_type.find("application/json") == 0 || Hreq.content_type == "text/plain")
            {
                std::cout << "app/json" << std::endl;
                if (Hreq.method == "POST")
                {
                    std::string file_path = generate_temp_file_path();
                    std::ofstream outfile(file_path.c_str());
                    if (outfile.is_open()) 
                    {
                        outfile << Hreq.body._body;
                        outfile.close();
                    }
                    Hreq.body._body.clear();
                }
                else {
                    Hreq.body._body.clear();
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
                // std::cout << "urlencoded1" << std::endl;

                std::string body_line;
                std::getline(dd, Hreq.body._body);

                std::stringstream pair_stream(Hreq.body._body);
                std::string pair;

                while (std::getline(pair_stream, pair, '&'))
                {
                    // std::cout << "urlencoded3" << std::endl;
                    std::cout << "cout = " <<  pair << std::endl;
                    size_t equal_pos = pair.find('=');
                    if (equal_pos != std::string::npos)
                    {
                        // std::cout << "urlencoded4" << std::endl;
                        std::string key = pair.substr(0, equal_pos);
                        std::string value = pair.substr(equal_pos + 1);
                        std::cout << "key = " << key << std::endl;
                        std::cout << "value = " << value << std::endl;
                        if (Hreq.method == "POST")
                        {
                            Hreq.body_map.insert(std::make_pair(key, value));
                        }
                    }
                }
                Hreq.body.chunk_done = true;
                request_received = true;
            }
            else if (Hreq.content_type.find("multipart/form-data") == 0)
            {
                size_t pos = Hreq.content_type.find("boundary=");
                if (pos == std::string::npos) 
                {
                    return PARSE_BAD_REQUEST;
                }
                boundary = "--" + Hreq.content_type.substr(pos + 9);
                std::cout << "boundary = " <<  boundary << std::endl;
                // boundary = "--" + Hreq.content_type.substr(Hreq.content_type.find("=") + 1,  Hreq.content_type.back()) + "\r\n";
                std::vector<std::string> body_vector = spliting(Hreq.body._body, boundary);
                std::cout << "multipart" << std::endl;

                std::string filename;
                std::string name;
                std::string content;
                std::cout << "Before content = " << content << std::endl;
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
                            if (Hreq.method == "POST")
                            {
                                std::string file_path = generate_temp_file_path();
                                std::ofstream outfile(file_path.c_str());
                                // std::cout << "part = " << part << std::endl;
                                if (outfile.is_open()) 
                                {
                                    outfile << part;
                                    outfile.close();
                                }
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
                                std::string body_part = part.substr(pos + 4);
                                std::cout << "body_part = " <<  body_part << std::endl;
                                size_t boundary_end = body_part.rfind("\r\n");
                                std::cout << " boundary_end = " << boundary_end << std::endl; 
                                if (boundary_end != std::string::npos)
                                {
                                    body_part = body_part.substr(0, boundary_end);
                                }
                                
                                if (Hreq.method == "POST" && is_cgi == false)
                                {
                                    file_path = generate_temp_file_path();
                                    std::ofstream outfile(file_path.c_str());
                                    if (outfile.is_open()) 
                                    {
                                        outfile << body_part;
                                        outfile.close();
                                    }
                                }
                                else {
                                    body_part.clear();
                                }
                                std::cout << "content = " << body_part;
                            }
                        }
                    }
                        std::cout << "body vector = " << *it << std::endl;
                        part.clear();
                    }
                    Hreq.body.chunk_done = true;
                    request_received = true;
                    filename.clear();
                    name.clear();
                    content.clear();
                    std::cout << "After content = " << content << std::endl;
                    boundary.clear();
                    body_vector.clear();
                    Hreq.body._body.clear();
                    read_buffer.clear();
                }
            }
        }
    
    if (request_received)
    {
        return PARSE_OK;
    }
    else
        return PARSE_INCOMPLETE;
}
