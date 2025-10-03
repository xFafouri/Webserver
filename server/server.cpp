#include "server.hpp"
#include <cstring>
#include <string>
#include <sys/socket.h>
#include <sys/types.h>

//extension

Server::Server(const ServerCo& conf) : config(conf)
{
    memset(&address, 0, sizeof(address));
    address.ai_family = AF_INET;
    address.ai_socktype = SOCK_STREAM;
    server_fd = socket(address.ai_family, address.ai_socktype, 0);
    if (server_fd < 0)
        throw std::runtime_error("Failed to create socket");

    // std::cout << "port = " << to_string(conf.listen).c_str() << std::endl;
    if (getaddrinfo(conf.host.c_str(), to_string(conf.listen).c_str(), &address, &res) != 0) 
    {
        std::cerr << "getaddrinfo: " << strerror(errno) << std::endl;
        exit(0);
    }

    int optval = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)) == -1)
    {
        std::cerr << "setsockopt: " << strerror(errno) << std::endl;
        close(server_fd);
        exit(0);
    }
    if (bind(server_fd, res->ai_addr, res->ai_addrlen))
    {
        std::cerr << "bind failed: " << strerror(errno) << std::endl;
        close(server_fd);
        exit(0); 
    }
    
    if (listen(server_fd, SOMAXCONN) < 0)
    {
        std::cerr << "listen: " << strerror(errno) << std::endl;
        close(server_fd);
        exit(0);
    }
    freeaddrinfo(res);
    epoll_fd = epoll_create(1);
    server_event.events = EPOLLIN | EPOLLET | EPOLLRDHUP;
    server_event.data.fd = server_fd;
    epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &server_event);
    addrlen = sizeof(address);
    std::cout << "Listening on " << conf.host << ":" << conf.listen << std::endl;
}

Client::Client()
{

    cgi_bin = false;
    request_received = false;
    response_sent = false;
    is_cgi = false;
    isGET = false;
    isPOST = false;
    isDELETE = false;

    cgi_pid       = -1;
    cgi_stdin_fd  = -1;
    cgi_stdout_fd = -1;
    cgi_stdin_off = 0;
    cgi_state     = CGI_IDLE;
    cgi_sep_len    = 0;
    cgi_content_len = -1;
    cgi_deadline_ms = 0;
    cgi_sep_pos    = std::string::npos;
    cgi_hdr_parsed = false;
    Hreq.header.parsed = false;
    Hreq.body.chunk_done = false;
    Hreq.body.is_done = false;
    Hreq.body.reading_chunk_size = true;
    is_sending_response = false;
    timed_out = false; 
    response_ready = false; 
    client_fd = -1; 
    status = PARSE_INCOMPLETE;
    Hreq.header.parsed = false;
    request_received = false;
    Hreq.body.chunk_done = false;
    Hreq.body.current_chunk_size = 0;
    Hreq.body.reading_chunk_size = true;
    response_sent = false;
    send_offset = 0;
    Hreq.body.chunked = false;
    Hreq.body.current_chunk_size = 0;
    Hreq.body.reading_chunk_size = true;
    timed_out = false;
    cgi_error_code = 0;


    header_sent = false;
    header_size = 0;
    // total = 0;
    // remaining = 0;
    // send_len = 0;
    last_write_time = 0;
    status_code = 0;
    to_close = 0;
    id_server = 0;
    env_map.clear();
    map_ext.clear();
    extension.clear();
    script_file.clear();
    Hreq.body.expected_size = 0;
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


void Client::reset_for_next_request()
{
    send_offset = 0;
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

     send_offset = 0;
    cgi_bin = false;
    request_received = false;
    response_sent = false;
    is_cgi = false;
    isGET = false;
    isPOST = false;
    isDELETE = false;

    cgi_pid       = -1;
    cgi_stdin_fd  = -1;
    cgi_stdout_fd = -1;
    cgi_stdin_off = 0;
    cgi_state     = CGI_IDLE;
    cgi_sep_len    = 0;
    cgi_content_len = -1;
    cgi_deadline_ms = 0;
    cgi_sep_pos    = std::string::npos;
    cgi_hdr_parsed = false;
    Hreq.header.parsed = false;
    Hreq.body.expected_size = 0;
    Hreq.body.chunk_done = false;
    Hreq.body.is_done = false;
    Hreq.body.reading_chunk_size = true;
    cgi_state = CGI_IDLE;
    Hreq.body.data.clear();
    Hreq.body.raw_body.clear();
    Hreq.body.expected_size = 0;
    Hreq.body.chunked = false;
    Hreq.body.is_done = false;
    Hreq.body.current_chunk_size = 0;
    Hreq.body.reading_chunk_size = true;
    timed_out = false;
}

void Client::handle_chunked_body(size_t max_body_size) 
{
    // std::cout << "handle_chunked_body" << std::endl;
    while (true) 
    {
        if (Hreq.body.reading_chunk_size) 
        {
            size_t line_end = Hreq.body._body.find("\r\n");
            if (line_end == std::string::npos) 
            {
                return;
            }
            std::string size_str = Hreq.body._body.substr(0, line_end);
            Hreq.body._body.erase(0, line_end + 2);

            Hreq.body.current_chunk_size = std::strtol(size_str.c_str(), NULL, 16);

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

std::string ft_content_of_extension(std::string content_type){
    if (content_type == ".html")
        return ".html";
    else if (content_type == ".css")
        return ".css";
    else if (content_type == ".js")
        return ".js";
    else if (content_type == ".png")
        return ".png";
    else if (content_type == ".jpeg")
        return ".jpg";  // could also be ".jpeg"
    else if (content_type == ".gif")
        return ".gif";
    else if (content_type == ".svg")
        return ".svg";
    else if (content_type == ".ico")
        return ".ico";
    else if (content_type == ".json")
        return ".json";
    else if (content_type == ".txt")
        return ".txt";
    else if (content_type == ".xml")
        return ".xml";
    else if (content_type == ".pdf")
        return ".pdf";
    else if (content_type == ".mp4")
        return ".mp4";
    else if (content_type == ".mp3")
        return ".mp3";
    else if (content_type == ".woff")
        return ".woff";
    else if (content_type == ".woff2")
        return ".woff2";
    else if (content_type == ".ttf")
        return ".ttf";
    else if (content_type == ".otf")
        return ".otf";
    else if (content_type == ".eot")
        return ".eot";
    else if (content_type == ".zip")
        return ".zip";
    else if (content_type == ".tar")
        return ".tar";
    else if (content_type == ".gz")
        return ".gz";
    else if (content_type == ".rar")
        return ".rar";
    else if (content_type == ".csv")
        return ".csv";
    else if (content_type == ".md")
        return ".md";
    else if (content_type == ".py")
        return ".py";
    else if (content_type == ".php")
        return ".php";
    else if (content_type == ".yaml")
        return ".yaml";  // could also be ".yml"
    else
        return ""; // Unknown type → no extension
}


std::string Client::ft_file_extension(const std::string& full_path)
{
    if (full_path.size() >= 5 && full_path.compare(full_path.size() - 5, 5, ".html") == 0)
        return ".html";
    else if (full_path.size() >= 4 && full_path.compare(full_path.size() - 4, 4, ".css") == 0)
        return ".css";
    else if (full_path.size() >= 3 && full_path.compare(full_path.size() - 3, 3, ".js") == 0)
        return ".js";
    else if (full_path.size() >= 4 && full_path.compare(full_path.size() - 4, 4, ".png") == 0)
        return ".png";
    else if ((full_path.size() >= 4 && full_path.compare(full_path.size() - 4, 4, ".jpg") == 0) ||
             (full_path.size() >= 5 && full_path.compare(full_path.size() - 5, 5, ".jpeg") == 0))
        return ".jpg"; // or ".jpeg"
    else if (full_path.size() >= 4 && full_path.compare(full_path.size() - 4, 4, ".gif") == 0)
        return ".gif";
    else if (full_path.size() >= 4 && full_path.compare(full_path.size() - 4, 4, ".svg") == 0)
        return ".svg";
    else if (full_path.size() >= 4 && full_path.compare(full_path.size() - 4, 4, ".ico") == 0)
        return ".ico";
    else if (full_path.size() >= 5 && full_path.compare(full_path.size() - 5, 5, ".json") == 0)
        return ".json";
    else if (full_path.size() >= 4 && full_path.compare(full_path.size() - 4, 4, ".txt") == 0)
        return ".txt";
    else if (full_path.size() >= 4 && full_path.compare(full_path.size() - 4, 4, ".xml") == 0)
        return ".xml";
    else if (full_path.size() >= 4 && full_path.compare(full_path.size() - 4, 4, ".pdf") == 0)
        return ".pdf";
    else if (full_path.size() >= 4 && full_path.compare(full_path.size() - 4, 4, ".mp4") == 0)
        return ".mp4";
    else if (full_path.size() >= 4 && full_path.compare(full_path.size() - 4, 4, ".mp3") == 0)
        return ".mp3";
    else if (full_path.size() >= 5 && full_path.compare(full_path.size() - 5, 5, ".woff") == 0)
        return ".woff";
    else if (full_path.size() >= 6 && full_path.compare(full_path.size() - 6, 6, ".woff2") == 0)
        return ".woff2";
    else if (full_path.size() >= 4 && full_path.compare(full_path.size() - 4, 4, ".ttf") == 0)
        return ".ttf";
    else if (full_path.size() >= 4 && full_path.compare(full_path.size() - 4, 4, ".otf") == 0)
        return ".otf";
    else if (full_path.size() >= 4 && full_path.compare(full_path.size() - 4, 4, ".eot") == 0)
        return ".eot";
    else if (full_path.size() >= 4 && full_path.compare(full_path.size() - 4, 4, ".zip") == 0)
        return ".zip";
    else if (full_path.size() >= 4 && full_path.compare(full_path.size() - 4, 4, ".tar") == 0)
        return ".tar";
    else if (full_path.size() >= 3 && full_path.compare(full_path.size() - 3, 3, ".gz") == 0)
        return ".gz";
    else if (full_path.size() >= 4 && full_path.compare(full_path.size() - 4, 4, ".rar") == 0)
        return ".rar";
    else if (full_path.size() >= 4 && full_path.compare(full_path.size() - 4, 4, ".csv") == 0)
        return ".csv";
    else if (full_path.size() >= 3 && full_path.compare(full_path.size() - 3, 3, ".md") == 0)
        return ".md";
    else if (full_path.size() >= 3 && full_path.compare(full_path.size() - 3, 3, ".py") == 0)
        return ".py";
    else if (full_path.size() >= 4 && full_path.compare(full_path.size() - 4, 4, ".php") == 0)
        return ".php";
    else if (full_path.size() >= 3 && full_path.compare(full_path.size() - 3, 3, ".pl") == 0)
        return ".pl";
    else if ((full_path.size() >= 5 && full_path.compare(full_path.size() - 5, 5, ".yaml") == 0) ||
             (full_path.size() >= 4 && full_path.compare(full_path.size() - 4, 4, ".yml") == 0))
        return ".yaml";
    else
        return ""; // unknown extension
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
        return "text/html";
    else if (full_path.find(".php") != std::string::npos)
        return "text/html";   
    else if (full_path.find(".pl") != std::string::npos)
        return "text/html";  
    else if (full_path.find(".yaml") != std::string::npos || full_path.find(".yml") != std::string::npos)
        return "application/x-yaml";
    else
        return "application/octet-stream"; // Default type for unknown extensions
}


bool directoryExists(const std::string &path) 
{
    struct stat info;
    if (stat(path.c_str(), &info) != 0)
        return false;
    return (info.st_mode & S_IFDIR) != 0;
}

bool createDirectory(const std::string &path) 
{
    if (mkdir(path.c_str(), 0755) == 0)
        return true;
    if (errno == EEXIST)
        return true; // already exists
    return false;
}

std::string generateUniqueName() 
{
    std::ostringstream oss;
    oss << "upload_" << std::time(NULL) << "_" << rand();
    return oss.str();
}

RequestParseStatus Client::read_from_fd(int client_fd, long long max_body_size)
{
    this->client_fd = client_fd;

    char recv_buffer[80000];
    ssize_t n = recv(client_fd, recv_buffer, sizeof(recv_buffer)-1, 0);
    if (n > 0) 
    {
        read_buffer.append(recv_buffer, n);
        last_activity = now_ms();
    } 
    else if (n == 0) 
    {
        return PARSE_CONNECTION_CLOSED;
    } 
    else 
    {
        return PARSE_INCOMPLETE;
    }
    // std::cout << "read_buffer = " << read_buffer << std::endl;
    const Location* matchedLocation = NULL;
    if (!Hreq.header.parsed) 
    {
        size_t header_end = read_buffer.find("\r\n\r\n");
        size_t sep_len = 4;
        if (header_end == std::string::npos) 
        {
            header_end = read_buffer.find("\n\n");
            sep_len = 2;
        }
        
        
        if (header_end == std::string::npos) 
        {
            return PARSE_INCOMPLETE;
        }
        
        // Find end of request line (CRLF or LF)
        size_t line_end = read_buffer.find("\r\n"); 
        size_t line_len = 2;
        if (line_end == std::string::npos) 
        {
            line_end = read_buffer.find('\n');
            line_len = 1;
        }
        if (line_end == std::string::npos || line_end > header_end) 
        {
            return PARSE_BAD_REQUEST;
        }
        
        std::string request_line = read_buffer.substr(0, line_end);
        // std::cout << "request_line = " <<  request_line << std::endl;
        std::istringstream rl(request_line);
        rl >> Hreq.method >> Hreq.uri >> Hreq.http_v;
        
        // std::cout << "uri before " << Hreq.uri << std::endl;
        Hreq.uri = normalize_path(Hreq.uri);
        // std::cout << "uri after " << Hreq.uri << std::endl;
        
        for (size_t i = 0; i < config.locations.size(); ++i)
        {
            if (Hreq.uri.find(config.locations[i].path) == 0)
            {
                if (!matchedLocation || config.locations[i].path.length() > matchedLocation->path.length())
                {
                    matchedLocation = &config.locations[i];
                    // std::cout << "match location " << std::endl;
                    file_path = matchedLocation->upload_store;             
                }
            }
        }
        if (!matchedLocation)
        {
            if (Hreq.uri.empty())
                return PARSE_BAD_REQUEST;
            else
                return REQUEST_NOT_FOUND;
        }
        if (Hreq.method == "GET") 
        {
            isGET = true;
        }
        if (Hreq.method == "DELETE")
        {
            isDELETE = true;
        }
        if (Hreq.uri.length() > 4096) 
        {
            return REQUEST_URI_TOO_LONG;
        }
        
        std::string headers_block = read_buffer.substr(line_end + line_len,header_end - (line_end + line_len));
        std::istringstream hs(headers_block);
        std::string header_line;
            while (std::getline(hs, header_line)) 
            {
                if (!header_line.empty() && header_line[header_line.size() - 1] == '\r')
                    header_line.erase(header_line.size() - 1);

                size_t colon = header_line.find(':');
                if (colon != std::string::npos) {
                    std::string key = header_line.substr(0, colon);
                    std::string value = header_line.substr(colon + 1);
                    trim(key); 
                    trim(value);
                    Hreq.map_header[key] = value;
            }
        }
                Hreq.header.parsed = true;
                if (Hreq.map_header.find("Host") == Hreq.map_header.end())
                {
                    return PARSE_BAD_REQUEST;
                }
                if (matchedLocation->path == "/cgi-bin" && !matchedLocation->cgi_flag)
                {
                    return PARSE_FORBIDDEN;
                }
                if (is_cgi_request()) 
                {
                    if (is_cgi_script(script_file)) 
                    {
                        if (access(script_file.c_str(), F_OK) != 0)
                        {
                            return REQUEST_NOT_FOUND; 
                        }
                        is_cgi = true;
                    } 
                    else 
                    {
                        is_cgi = false;
                        map_ext.clear();
                        script_file.clear();
                    }
                }
                              
                bool post_method = false;
        for (size_t index = 0; index < matchedLocation->allowed_methods.size(); index++)
        {
            if (Hreq.method == matchedLocation->allowed_methods[index])
                post_method = true;
        }
        // std::cout << "Hreq method -->>> " << Hreq.method << std::endl;
        if (post_method == false)
        {
            return REQUEST_METHOD_NOT_ALLOWED;
        }

        if (Hreq.method == "POST") 
        {
            if (Hreq.map_header.find("Content-Length") != Hreq.map_header.end()) 
            {
                cl = Hreq.map_header["Content-Length"];
                Hreq.body.expected_size = std::atoll(cl.c_str());
                Hreq.content_length = Hreq.body.expected_size ;
                // std::cout << "max_body_size = " << max_body_size << std::endl;
                // std::cout << "expected_size = " << Hreq.body.expected_size << std::endl;
                if (Hreq.body.expected_size > (static_cast<size_t>(config.client_max_body_size)))
                    return PAYLOAD_TOO_LARGE;
                if (Hreq.map_header.find("Content-Type") != Hreq.map_header.end()) 
                {
                    Hreq.content_type = trim(Hreq.map_header["Content-Type"]);
                } 
                else 
                {
                    return PARSE_BAD_REQUEST;
                }
            } 
            else if (Hreq.map_header.find("Transfer-Encoding") != Hreq.map_header.end()
                        && Hreq.map_header["Transfer-Encoding"].find("chunked") != std::string::npos) {
                Hreq.body.chunked = true;
            } 
            else 
            {
                return PARSE_BAD_REQUEST;
            }
        }
        size_t body_start = header_end + sep_len;
        if (body_start < read_buffer.size()) 
        {
            Hreq.body._body.append(read_buffer.substr(body_start));
        }
        read_buffer.clear();

        if (!Hreq.body.chunked && Hreq.body.expected_size == 0) 
        {
            request_received = true;
            return PARSE_OK;
        }

        if (Hreq.body._body.size() > (static_cast<size_t>(config.client_max_body_size))) 
        {
            return PAYLOAD_TOO_LARGE;
        }
    } 
    else 
    {
        if (!read_buffer.empty()) 
        {
            Hreq.body._body.append(read_buffer);
            read_buffer.clear();
            if (Hreq.body._body.size() > (static_cast<size_t>(config.client_max_body_size)))
                return PAYLOAD_TOO_LARGE;
        }
    }

    for (size_t i = 0; i < config.locations.size(); ++i)
    {
        if (Hreq.uri.find(config.locations[i].path) == 0)
        {
            if (!matchedLocation || config.locations[i].path.length() > matchedLocation->path.length())
            {
                matchedLocation = &config.locations[i];
                // std::cout << "match location " << std::endl;
                file_path = matchedLocation->upload_store;             
            }
        }
    }
    if (!matchedLocation)
    {
            return REQUEST_NOT_FOUND;
    }

    // std::cout << Hreq.header.parsed << " header parsed" <<  std::endl;
    if (Hreq.map_header.empty() &&  Hreq.header.parsed)
    {
        return PARSE_BAD_REQUEST;
    }
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
                if (!matchedLocation->upload_flag) 
                {
                    // std::cerr << "Upload attempted but not allowed." << std::endl;
                    return PARSE_FORBIDDEN;
                }
                // make sure upload dir exists
                if (!directoryExists(file_path)) 
                {
                    if (!createDirectory(file_path)) 
                    {
                        std::cerr << "Failed to create upload directory: " 
                                  << file_path << std::endl;
                        return PARSE_INTERNAL_ERROR;
                    }
                }
                // std::cout << "content type -->>>> " << Hreq.content_type << std::endl;
                extension = ft_file_extension(Hreq.uri);
                std::string full_file_path = file_path + "/" + generateUniqueName() + extension;
                // std::cout << "full_file_path ===========> " << full_file_path << std::endl;           
                std::ofstream outfile(full_file_path.c_str(), std::ios::binary);
                if (!outfile.is_open()) {
                    std::cerr << "Failed to open file for writing: " 
                              << full_file_path << std::endl;
                    return PARSE_INTERNAL_ERROR;
                }
                // std::cout << "body.data = " << Hreq.body.data << std::endl;
                isPOST = true;
                outfile << Hreq.body.data;
                outfile.close();
                Hreq.body.reset();
            }
            request_received = true;
            return PARSE_OK;
        }
        else 
        {
            return PARSE_INCOMPLETE;
        }
    }

    if (Hreq.body.expected_size > 0) 
    {
        // std::cout << "Body_size = "<< Hreq.body._body.size() << std::endl;
        if (Hreq.body._body.size() > Hreq.body.expected_size)
        {
            return PARSE_BAD_REQUEST;
        }
        if (Hreq.body._body.size() < Hreq.body.expected_size ) 
        {
            return PARSE_INCOMPLETE;
        }
        // std::cout <<  "Body = " << Hreq.body._body << std::endl;
        if (Hreq.method == "POST") 
        {
                // std::cout << "file_path  ="  <<  file_path << std::endl;
                if (!matchedLocation->upload_flag) 
                {
                    // std::cerr << "Upload attempted but not allowed." << std::endl;
                    return PARSE_FORBIDDEN;
                }
                if (!directoryExists(file_path))
                {
                    if (!createDirectory(file_path)) 
                    {
                        std::cerr << "Failed to create upload directory: " 
                                  << file_path << std::endl;
                        return PARSE_INTERNAL_ERROR;
                    }
                }
                // std::cout << "extension = > " << extension << std::endl;
                // std::cout << "content type -->>>> " << Hreq.content_type << std::endl;
                extension = ft_file_extension(Hreq.uri);
                std::string full_file_path = file_path + "/" + generateUniqueName() + extension;
                // std::cout << "=======> full_file_path  ="  <<  full_file_path << std::endl;
                // std::cout << "=======> file_path  ="  <<  file_path << std::endl;
                std::ofstream outfile(full_file_path.c_str(), std::ios::binary);
                if (!outfile.is_open()) 
                {
                    std::cerr << "Failed to open file for writing: " 
                              << full_file_path << std::endl;
                    return PARSE_INTERNAL_ERROR;
                }
            
                // std::cout << "body.data = " << Hreq.body._body << std::endl;
                outfile << Hreq.body._body;
                isPOST = true;
                outfile.close();
                Hreq.body.reset();
        }

        request_received = true;
        return PARSE_OK;
    }
    return PARSE_INCOMPLETE;
}
