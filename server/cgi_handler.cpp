
#include "server.hpp"
#include <cstddef>
#include <sys/types.h>

std::string ft_content_type_cgi(const std::string& full_path)
{
    if (full_path.find(".py") != std::string::npos)
        return ".py";
    else if (full_path.find(".php") != std::string::npos)
        return ".php";
    else if (full_path.find(".pl") != std::string::npos)
        return ".pl";
    else return "";
}

bool Client::is_cgi_script(const std::string &path) 
{
    std::vector<std::string> cgi_exts;
    cgi_exts.push_back(".py");
    cgi_exts.push_back(".php");
    cgi_exts.push_back(".pl");
    
    if (extension ==  ".py" || extension == ".php" || extension == ".pl")
        return true;
    for (size_t i = 0; i < cgi_exts.size(); i++) 
    {
        if (path.size() >= cgi_exts[i].size() &&
            path.compare(path.size() - cgi_exts[i].size(), cgi_exts[i].size(), cgi_exts[i]) == 0) 
            {
                return true;
            }
    }
    return false;
}

bool Client::is_cgi_request() 
{
    const Location* matchedLocation = NULL;
    
    for (size_t i = 0; i < config.locations.size(); ++i) 
    {
        if (Hreq.uri.find(config.locations[i].path) == 0) 
        {
            if (!matchedLocation || config.locations[i].path.length() > matchedLocation->path.length()) 
            {
                matchedLocation = &config.locations[i];
                location_idx = i;
            }
        }
    }
    // std::cout << "MMatchedLocation = " << matchedLocation->path << std::endl;
    if (!matchedLocation)
        return false;
    if (Hreq.uri == matchedLocation->path || Hreq.uri == matchedLocation->path + "/") 
    {
        for (size_t i = 0; i < matchedLocation->index_files.size(); ++i) 
        {
            std::string index_path = matchedLocation->root + "/" + matchedLocation->index_files[i];
            // std::cout << "index path ==><><> " << index_path << std::endl;
            if (access(index_path.c_str(), F_OK) == 0) 
            {
                script_file = index_path;
                // std::cout << script_file << std::endl;
                extension = ft_content_type_cgi(script_file);
                cgi_bin = true;
                map_ext[".php"] = matchedLocation->cgi.php;
                map_ext[".py"]  = matchedLocation->cgi.py;
                map_ext[".pl"]  = matchedLocation->cgi.pl;
                return true;
            }
        }
        return false; 
    }

    {
        if (matchedLocation->path != "/cgi-bin")
            return false;
        std::string relative_uri = Hreq.uri.substr(matchedLocation->path.length());
        if (!relative_uri.empty() && relative_uri[0] == '/')
            relative_uri.erase(0, 1);

        script_file = matchedLocation->root + "/" + relative_uri;
        // std::cout << "script file here = " <<  script_file << std::endl;
        size_t dot_pos = script_file.rfind('.');
        size_t query_pos = script_file.find('?');
        // std::cout << "pos = " << dot_pos << std::endl;
        // std::cout << "query_pos = " << query_pos << std::endl;
        if (dot_pos == std::string::npos)
            return false;
        if (query_pos != std::string::npos)
        {
            extension = script_file.substr(dot_pos,query_pos - dot_pos);
            extension = script_file.substr(dot_pos);
            script_file = script_file.substr(0,query_pos);
            // std::cout << "extension1 = " << extension <<  std::endl;
        }
        else 
        {
            extension = script_file.substr(dot_pos);
            // std::cout << "extension = " << extension <<  std::endl;
        
        }
        // std::cout << "extenson = " << extension <<  std::endl;
        map_ext[".php"] = matchedLocation->cgi.php;
        map_ext[".py"]  = matchedLocation->cgi.py;
        map_ext[".pl"]  = matchedLocation->cgi.pl;

        if (map_ext.find(extension) != map_ext.end()) 
        {
            cgi_bin = true;
            return true;
        }
        return (extension == ".py" || extension == ".php" || extension == ".pl");
    }
}

static inline void nb_set(int fd) 
{
    int fl = fcntl(fd, F_GETFL, 0);
    if (fl >= 0) fcntl(fd, F_SETFL, fl | O_NONBLOCK);
}
static size_t parse_header_block(const std::string& hdrs,
                                 std::string& status_line,
                                 std::string& content_type,
                                 ssize_t& content_len)
{
    size_t consumed = hdrs.size();
    content_len = -1;
    content_type.clear();

    size_t start = 0;
    while (start < hdrs.size()) 
    {
        size_t end = hdrs.find('\n', start);
        if (end == std::string::npos) 
            end = hdrs.size();
        std::string line = hdrs.substr(start, end - start);
        if (!line.empty() && line[line.size() - 1] == '\r')
            line.erase(line.size() - 1);

        if (!line.empty()) 
        {
            if (line.size() >= 7 && strncasecmp(line.c_str(), "Status:", 7) == 0) 
            {
                std::string v = line.substr(7);
                // trim leading spaces
                while (!v.empty() && (v[0] == ' ' || v[0] == '\t')) v.erase(0, 1);
                if (!v.empty()) status_line = v; // e.g. "404 Not Found"
            } 
            else if (line.size() >= 13 && strncasecmp(line.c_str(), "Content-Type:", 13) == 0) 
            {
                std::string v = line.substr(13);
                while (!v.empty() && (v[0] == ' ' || v[0] == '\t')) 
                    v.erase(0, 1);
                if (!v.empty()) 
                    content_type = v;
            } 
            else if (line.size() >= 15 && strncasecmp(line.c_str(), "Content-Length:", 15) == 0)
            {
                std::string v = line.substr(15);
                while (!v.empty() && (v[0] == ' ' || v[0] == '\t')) v.erase(0, 1);
                if (!v.empty()) content_len = static_cast<ssize_t>(atoll(v.c_str()));
            }
        }

        if (end == hdrs.size()) 
            break;
        start = end + 1;
    }
    return consumed;
}

void Client::finalize_cgi(bool eof_seen)
{
    cgi_state = CGI_DONE;

    (void)eof_seen;
    int status;
    waitpid(cgi_pid, &status, WNOHANG);
    cgi_pid = -1;
    std::string headers, body;
    if (!cgi_hdr_parsed) 
    {
        headers.clear();
        body = cgi_raw;
        cgi_content_type = "text/html";
        cgi_content_len = static_cast<ssize_t>(body.size());
    } 
    else 
    {
        headers = cgi_raw.substr(0, cgi_sep_pos);
        body    = (cgi_sep_pos + cgi_sep_len <= cgi_raw.size())
                    ? cgi_raw.substr(cgi_sep_pos + cgi_sep_len)
                    : std::string();

        if (cgi_content_len < 0) 
        {
            cgi_content_len = static_cast<ssize_t>(body.size());
        } 
        else 
        {
            if (body.size() > static_cast<size_t>(cgi_content_len))
                body.resize(static_cast<size_t>(cgi_content_len));
        }
        if (cgi_content_type.empty())
            cgi_content_type = "text/html";
    }

    std::ostringstream oss;
    // Status line
    oss << "HTTP/1.1 " << (cgi_status_line.empty() ? "200 OK" : cgi_status_line) << "\r\n";

    {
        if (!headers.empty()) {
            std::istringstream hs(headers);
            std::string line;
            while (std::getline(hs, line)) {
               if (!line.empty() && line[line.size() - 1] == '\r')
                    line.erase(line.size() - 1);
                if (line.empty()) continue;
                // filter
                if (!strncasecmp(line.c_str(), "Status:", 7) ||
                    !strncasecmp(line.c_str(), "Content-Length:", 15) ||
                    !strncasecmp(line.c_str(), "Content-Type:", 13)) {
                    continue;
                }
                oss << line << "\r\n"; 
            }
        }
    }

    oss << "Content-Type: " << cgi_content_type << "\r\n";
    oss << "Content-Length: " << cgi_content_len << "\r\n";
    oss << "\r\n";
    oss.write(body.data(), body.size());

    response_buffer = oss.str();
    send_offset = 0;            
    // response_ready = true;     
}

void Client::refresh_deadline()
{
    uint64_t current_time = now_ms();
    const uint64_t CGI_WINDOW_MS = 5000;
    cgi_deadline_ms = current_time + CGI_WINDOW_MS;
}

void Client::on_cgi_event(int epoll_fd, int fd, uint32_t events)
{
    if (cgi_state == CGI_ERROR || cgi_state == CGI_DONE || cgi_state == CGI_TIMED_OUT) 
        return;

    // refresh_deadline();
    if (fd == cgi_stdin_fd) 
    {
        if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR))
        {
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cgi_stdin_fd, NULL);
            close(cgi_stdin_fd);
            cgi_stdin_fd = -1;
        } 
        else if (events & EPOLLOUT) 
        {
            const std::string& body = Hreq.body._body;
            if (cgi_stdin_off < body.size()) 
            {
                ssize_t n = write(cgi_stdin_fd, body.data() + cgi_stdin_off,
                                  std::min<size_t>(8192, body.size() - cgi_stdin_off));
                if (n > 0) 
                {
                    cgi_stdin_off += static_cast<size_t>(n);
                    refresh_deadline();
                }
            }
            if (cgi_stdin_off >= body.size()) 
            {
                epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cgi_stdin_fd, NULL);
                close(cgi_stdin_fd);
                cgi_stdin_fd = -1;
            }
        }
    }

    if (fd == cgi_stdout_fd) 
    {
        if (events & EPOLLIN) 
        {
            char buf[8192];
            while (true) 
            {
                ssize_t n = read(cgi_stdout_fd, buf, sizeof(buf));
                if (n > 0) 
                {
                    cgi_raw.append(buf, n);
                    refresh_deadline();
                    if (!cgi_hdr_parsed) 
                    {
                        size_t p = cgi_raw.find("\r\n\r\n");
                        size_t sep_len = 4;
                        if (p == std::string::npos) 
                        {
                            p = cgi_raw.find("\n\n");
                            sep_len = 2;
                        }
                        if (p != std::string::npos) 
                        {
                            cgi_hdr_parsed = true;
                            cgi_sep_pos = p;
                            cgi_sep_len = sep_len;
                            const std::string hdrs = cgi_raw.substr(0, cgi_sep_pos);
                            parse_header_block(hdrs, cgi_status_line, cgi_content_type, cgi_content_len);
                        }
                    }
                    if (cgi_hdr_parsed && cgi_content_len >= 0) 
                    {
                        size_t have_body = (cgi_raw.size() > cgi_sep_pos + cgi_sep_len)
                                             ? (cgi_raw.size() - (cgi_sep_pos + cgi_sep_len))
                                             : 0;
                        if (have_body >= static_cast<size_t>(cgi_content_len)) 
                        {
                            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cgi_stdout_fd, NULL);
                            close(cgi_stdout_fd);
                            cgi_stdout_fd = -1;
                            finalize_cgi(false);
                            return;
                        }
                    }
                } 
                else if (n == 0) 
                {
                    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cgi_stdout_fd, NULL);
                    close(cgi_stdout_fd);
                    cgi_stdout_fd = -1;
                    finalize_cgi(true);
                    return;
                } 
                else {
                    break;
                }
            }
        }

        if (events & (EPOLLRDHUP | EPOLLHUP | EPOLLERR)) 
        {
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cgi_stdout_fd, NULL);
            close(cgi_stdout_fd);
            cgi_stdout_fd = -1;
            finalize_cgi(true);
            return;
        }
    }
}


void Client::abort_cgi(int epoll_fd)
{
    if (cgi_pid > 0) 
    {
        kill(cgi_pid, SIGKILL);
        int status; 
        waitpid(cgi_pid, &status, 0);
        cgi_pid = -1;
    }
    if (cgi_stdin_fd != -1) 
    {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cgi_stdin_fd, NULL);
        close(cgi_stdin_fd);
        cgi_stdin_fd = -1;
    }
    if (cgi_stdout_fd != -1) 
    {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cgi_stdout_fd, NULL);
        close(cgi_stdout_fd);
        cgi_stdout_fd = -1;
    }
    cgi_state = CGI_TIMED_OUT;

    const char* msg = "Gateway Timeout";
    std::ostringstream oss;
    oss << "HTTP/1.1 504 Gateway Timeout\r\n"
        << "Content-Type: text/plain\r\n"
        << "Content-Length: " << strlen(msg) << "\r\n"
        << "\r\n"
        << msg;

    response_buffer = oss.str();
    send_offset     = 0;
    response_ready  = true;
}


void Client::cleanup_cgi_fds(int epoll_fd) 
{
    if (cgi_stdin_fd != -1) 
    {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cgi_stdin_fd, NULL);
        close(cgi_stdin_fd);
        cgi_stdin_fd = -1;
    }
    if (cgi_stdout_fd != -1) 
    {
        epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cgi_stdout_fd, NULL);
        close(cgi_stdout_fd);
        cgi_stdout_fd = -1;
    }
    if (cgi_pid > 0) 
    {
        kill(cgi_pid, SIGKILL);
        waitpid(cgi_pid, NULL, 0);
        cgi_pid = -1;
    }
}

bool Client::is_valid_fd(int fd) 
{
    struct stat st;
    return fstat(fd, &st) == 0;
}
bool    Client::run_cgi(int epoll_fd)
{

    if (cgi_state == CGI_ERROR || cgi_state == CGI_DONE || cgi_state == CGI_TIMED_OUT) 
        return false ;

    if ((client_fd == cgi_stdin_fd && !is_valid_fd(cgi_stdin_fd)) ||
        (client_fd == cgi_stdout_fd && !is_valid_fd(cgi_stdout_fd))) 
    {
        cleanup_cgi_fds(epoll_fd);
        cgi_state = CGI_ERROR;
        return false ;
    }

    std::string query_string;
    env_map.clear();
    env_map.insert(std::make_pair("REQUEST_METHOD", Hreq.method));
    env_map.insert(std::make_pair("SERVER_PROTOCOL", "HTTP/1.1"));
    env_map.insert(std::make_pair("SCRIPT_FILENAME", script_file));
    if (Hreq.map_header.find("Cookie") != Hreq.map_header.end())
        env_map.insert(std::make_pair("HTTP_COOKIE", Hreq.map_header["Cookie"]));
    // std::cout << Hreq.map_header["Cookie"]<< std::endl;
    if (Hreq.method == "GET")
    {
        size_t dot_pos = Hreq.uri.find('?');
        if (dot_pos != std::string::npos)
            query_string = Hreq.uri.substr(dot_pos + 1);
        env_map["QUERY_STRING"] = query_string;
        query_string = Hreq.uri.substr(dot_pos + 1);
        env_map.insert(std::make_pair("QUERY_STRING", query_string));
        //querystring
    }
    else if (Hreq.method == "POST")
    {
        env_map.insert(std::make_pair("CONTENT_TYPE", Hreq.content_type));
        env_map.insert(std::make_pair("CONTENT_LENGTH", cl));

    }
    env_map.insert(std::make_pair("REDIRECT_STATUS", "200"));


    char **envp = new char*[env_map.size() + 1];

    std::map<std::string, std::string>::iterator it = env_map.begin();
    int i = 0;
    for(; it != env_map.end(); it++)
    {
        std::string entry = it->first + "=" + it->second;
        envp[i] = new char[entry.size() + 1];
        strcpy(envp[i], entry.c_str());
        i++;
    }
    envp[i] = NULL;
    
    i = 0;
    while(envp[i])
    {
        i++;
    }
    char *argv[] = 
    {
        (char*)map_ext[extension].c_str(),     
        (char*)script_file.c_str(),        
        NULL
    };
    int in_pipe[2];
    int out_pipe[2];
    if (pipe(in_pipe) == -1)
    {
        return false;
    }
    if (pipe(out_pipe) == -1) 
    { 
        close(in_pipe[0]); 
        close(in_pipe[1]); 
        return false;
    }
    if (access(map_ext[extension].c_str(), X_OK) == -1 || access(script_file.c_str(), R_OK) == -1) 
    {
        for (size_t j = 0; j < env_map.size(); ++j) delete[] envp[j];
            delete[] envp;
        cgi_error_code = 500;
        cgi_state = CGI_ERROR;
        cleanup_cgi_fds(epoll_fd);
        return false;
    }
    int pid = fork();
      if (pid == -1) 
    {
        close(in_pipe[0]); close(in_pipe[1]);
        close(out_pipe[0]); close(out_pipe[1]);
        return false;
    }

    if (pid == 0)
    {
        dup2(in_pipe[0], STDIN_FILENO);
        dup2(out_pipe[1], STDOUT_FILENO);

        for (int fd = 3; fd < 1024; ++fd) 
            close(fd);
        close(in_pipe[1]);
        close(out_pipe[0]);

        execve(argv[0], argv, envp);
        cleanup_cgi_fds(epoll_fd);
        _exit(127);
    }

    cgi_pid       = pid;
    cgi_stdin_fd  = in_pipe[1];
    cgi_stdout_fd = out_pipe[0];

    close(in_pipe[0]);
    close(out_pipe[1]);

    nb_set(cgi_stdin_fd);
    nb_set(cgi_stdout_fd);

    epoll_event ev;

    ev.events = EPOLLIN | EPOLLRDHUP | EPOLLHUP | EPOLLERR;
    ev.data.fd = cgi_stdout_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cgi_stdout_fd, &ev) == -1) 
    {
        close(cgi_stdin_fd); 
        close(cgi_stdout_fd);
        cgi_stdin_fd = cgi_stdout_fd = -1;
        return false;
    }

    if (Hreq.method == "POST" && !Hreq.body._body.empty()) 
    {
        ev.events = EPOLLOUT | EPOLLRDHUP | EPOLLHUP | EPOLLERR;
        ev.data.fd = cgi_stdin_fd;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, cgi_stdin_fd, &ev) == -1) {
            // cleanup
            epoll_ctl(epoll_fd, EPOLL_CTL_DEL, cgi_stdout_fd, NULL);
            close(cgi_stdin_fd); close(cgi_stdout_fd);
            cgi_stdin_fd = cgi_stdout_fd = -1;
            return false;
        }
    } 
    else 
    {
        close(cgi_stdin_fd);
        cgi_stdin_fd = -1;
    }
    for (int j = 0; j < i; ++j) 
        delete[] envp[j];
    delete[] envp;

     // initialize CGI state
    cgi_start_time = now_ms();
    cgi_deadline_ms = now_ms() + 5000; // e.g. CGI_TIMEOUT_MS ms
    cgi_state = CGI_IO;
    
    cgi_raw.clear();
    cgi_hdr_parsed = false;
    cgi_sep_pos = std::string::npos;
    cgi_sep_len = 0;
    cgi_status_line.clear();
    cgi_content_type.clear();
    cgi_content_len = -1;
    cgi_stdin_off = 0;

 
    return true;

}
