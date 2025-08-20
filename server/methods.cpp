#include "server.hpp"
#include <cstddef>
#include <sys/types.h>

void Client::getMethod()
{
    const Location* matchedLocation = NULL;
    for (size_t i = 0; i < config.locations.size(); ++i)
    {
        if (Hreq.uri.find(config.locations[i].path) == 0)
        {
            if (!matchedLocation || config.locations[i].path.length() > matchedLocation->path.length())
            {
                matchedLocation = &config.locations[i];
            }
        }
    }

    std::string response;
    if (!matchedLocation)
    {
        response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        send(client_fd, response.c_str(), response.size(), 0);
        return;
    }

    std::string location_path = matchedLocation->path;
    std::string relative_uri = Hreq.uri.substr(location_path.length());

    if (!relative_uri.empty() && relative_uri[0] == '/')
        relative_uri.erase(0, 1);

    std::string full_dir_path = matchedLocation->root + location_path;
    if (!full_dir_path.empty() && full_dir_path.back() == '/')
        full_dir_path.pop_back(); // avoid double slashes

    std::string full_path = full_dir_path;
    if (!relative_uri.empty())
        full_path += "/" + relative_uri;

    struct stat st;
    if (stat(full_path.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
    {
        // REDIRECT: If URI does not end with '/' but is a directory
        if (Hreq.uri.back() != '/')
        {
            std::string redirect_uri = Hreq.uri + "/";
            response = "HTTP/1.1 301 Moved Permanently\r\n";
            response += "Location: " + redirect_uri + "\r\n";
            response += "Content-Length: 0\r\n\r\n";
            send(client_fd, response.c_str(), response.size(), 0);
            return;
        }

        std::string index_path;
        bool foundIndex = false;

        for (size_t i = 0; i < matchedLocation->index_files.size(); ++i)
        {
            index_path = full_path + "/" + matchedLocation->index_files[i];
            if (access(index_path.c_str(), F_OK) == 0)
            {
                foundIndex = true;
                break;
            }
        }

        if (foundIndex)
        {
            std::ifstream file(index_path.c_str());
            if (!file)
            {
                response = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
                send(client_fd, response.c_str(), response.size(), 0);
                return;
            }

            std::ostringstream ss;
            ss << file.rdbuf();
            std::string body = ss.str();
            response = "HTTP/1.1 200 OK\r\n";
            response += "Content-Type: text/html\r\n";
            response += "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n";
            response += body;
            send(client_fd, response.c_str(), response.size(), 0);
            return;
        }
        else if (matchedLocation->autoindex)
        {
            response = "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\n\r\n";
            response += "<html><body><h1>Autoindex not implemented yet</h1></body></html>";
            send(client_fd, response.c_str(), response.size(), 0);
            return;
        }
        else
        {
            response = "HTTP/1.1 403 Forbidden\r\nContent-Length: 0\r\n\r\n";
            send(client_fd, response.c_str(), response.size(), 0);
            return;
        }
    }
    else if (stat(full_path.c_str(), &st) == 0 && S_ISREG(st.st_mode))
    {
        std::ifstream file(full_path.c_str());
        if (!file)
        {
            response = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
            send(client_fd, response.c_str(), response.size(), 0);
            return;
        }

        std::ostringstream ss;
        ss << file.rdbuf();
        std::string body = ss.str();
        std::string content_type = ft_content_type(full_path);
        response_buffer = "HTTP/1.1 200 OK\r\n";
        response_buffer += "Content-Type: " + content_type + "\r\n";
        response_buffer += "Content-Length: " + std::to_string(body.size()) + "\r\n\r\n";
        // ssize_t header_size = response_buffer.size ();
        // ssize_t total = 0;
        response_buffer += body;
        // ssize_t chunk_size = 8192;
        // ssize_t remaining = response_buffer.size() + header_size - send_offset;
        // std::cout << "remaining = " << remaining << std::endl;
        // if (remaining == 0) 
        // {
        //     // Done sending, close if needed
        //     // close(client_fd);
        //     return;
        // }

        // size_t to_send = std::min(chunk_size, remaining);
        // // ssize_t n = send(client_fd, response_buffer.data() + send_offset, to_send, 0);
        // std::cout << "send ====>" << n << std::endl;
        // if (n > 0) 
        // {
        //     send_offset += n;
        // }
        // else if (n < 0 && (errno != EWOULDBLOCK && errno != EAGAIN)) 
        // {

        //     // Error: close connection
        //     // close(client_fd);
        //     return ;
        // }
        // while (true)
        // {
        //     ssize_t n = send(client_fd, response.c_str(), response.size(), 0);
        //     std::cout << "N =====> = " << n << std::endl;
        //     if (n <= 0)
        //         return ;
        //     total += n;
        //     if (total >= body.size() + header_size)
        //     {
        //         std::cout << "total = " << total << std::endl;
        //         std::cout << body.size() << std::endl;
        //     break;
        //     }
        // }
        return;
    }
    else
    {
        response = "HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n";
        send(client_fd, response.c_str(), response.size(), 0);
        return;
    }
}


void Client::deleteMethod()
{

}