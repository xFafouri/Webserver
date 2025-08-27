#include "server.hpp"
#include <cstddef>
#include <sys/types.h>

std::string Client::normalize_path(const std::string &path)
{
    std::string result;
    bool check = false;
    for (size_t i = 0;i <  path.size(); i++)
    {
        char c = path[i];
        if (c == '/')
        {
            if (check == false)
            {
                result += c;
                check = true;
            }
        }
        else
        {
            result += c;
            check = false;
        }
    }
    return result;
}

// void Client::getMethod()
// {
//     const Location* matchedLocation = NULL;
//     Hreq.uri = normalize_path(Hreq.uri);
//     std::cout << "hreq uri == >> "<< Hreq.uri << std::endl;
std::string loadErrorPage(const std::string& path) {
    std::ifstream file(path.c_str());
    if (!file.is_open()) {
        // fallback default page
        std::stringstream fallback;
        fallback << "<!DOCTYPE html><html><head><title>Error</title></head>"
                 << "<body><h1>Error</h1><p>WebServerhelp/1.1</p></body></html>";
        return fallback.str();
    }
    std::cout << "heerrreeeeee -------------------- \n";
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void Client::sendError(int code)
{
    std::string status_text;
    switch(code) {
        case 400: status_text = "Bad Request"; break;
        case 403: status_text = "Forbidden"; break;
        case 404: status_text = "Not Found"; break;
        case 405: status_text = "Method Not Allowed"; break;
        case 408: status_text = "Request Timeout"; break;
        case 411: status_text = "Length Required"; break;
        case 413: status_text = "Payload Too Large"; break;
        case 414: status_text = "URI Too Long"; break;
        case 415: status_text = "Unsupported Media Type"; break;
        case 500: status_text = "Internal Server Error"; break;
        case 501: status_text = "Not Implemented"; break;
        case 504: status_text = "Gateway Timeout"; break;
        case 505: status_text = "HTTP Version Not Supported"; break;
        default: status_text = "Unknown Error"; break;
    }

    std::string body;
    std::map<int,std::string>::iterator it = config.error_pages.find(code);
    if (it != config.error_pages.end()) {
        // Key exists → use the HTML path
        body = loadErrorPage(it->second);
    } else {
        // Key does NOT exist → default page
        std::ostringstream oss;
        oss << "<!DOCTYPE html>"
            << "<html>"
            << "<head><title>" << code << " " << status_text << "</title></head>"
            << "<body>"
            << "<center><h1>" << code << " " << status_text << "</h1></center>"
            << "<hr><center>webserv 'omar hamza5'</center>"
            << "</body>"
            << "</html>";
        body = oss.str();
    }

    // Build headers
    std::ostringstream headers;
    headers << "HTTP/1.1 " << code << " " << status_text << "\r\n"
            << "Content-Type: text/html\r\n"
            << "Content-Length: " << body.size() << "\r\n\r\n";

    std::string response = headers.str() + body;
    send(client_fd, response.c_str(), response.size(), 0);
}


void Client::getMethod()
{
    const Location* matchedLocation = nullptr;
    Hreq.uri = normalize_path(Hreq.uri);

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

    std::cout << "matchedlocation == " << matchedLocation->path << std::endl;
    std::string response;
    if (!matchedLocation)
    {
        std::cerr << "dkhl hna" << std::endl;
        response = "HTTP/1.1 404 Not Found\r\n\r\n\r\n";
        response+= "<center><h1> ERROR 404</h1> <hr> <p>WebServer/1.1</p></center>";
        send(client_fd, response.c_str(), response.size(), 0);
        return;
    }

    std::string location_path = matchedLocation->path;
    std::cout << "location path == " << location_path << std::endl;
    std::string relative_uri = Hreq.uri.substr(location_path.length());
    std::cout << "relative path ==" << relative_uri << std::endl;

    if (!relative_uri.empty() && relative_uri[0] == '/')
        relative_uri.erase(0, 1);

    std::string full_dir_path = matchedLocation->root;
    if (full_dir_path.back() == '/')
        full_dir_path.pop_back();
    std::cout << "full_dir_path check / == " << full_dir_path.back() << std::endl;
    std::cout << "full dir path == " << full_dir_path << std::endl;
    if (!full_dir_path.empty() && full_dir_path.back() == '/')
        full_dir_path.pop_back(); // avoid double slashes

    std::string full_path = full_dir_path;
    if (!relative_uri.empty())
        full_path += "/" + relative_uri;
    std::cout << "full path === " << full_path << std::endl;

    struct stat st;
    if (stat(full_path.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
    {
        std::cout << "is a directory " << std::endl;
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
        response_buffer += body;
        // send(client_fd, response.c_str(), response.size(), 0);
        return;
    }
    else
    {
        response = "HTTP/1.1 404 Not Found\r\n";
        response += "Content-Type: text/html\r\n";
        response += "Content-Length: ";

        std::string body = 
        "<!DOCTYPE html>"
        "<html>"
        "<head><title>404 Not Found</title></head>"
        "<body>"
        "<center><h1>404 Not Found</h1></center>"
        "<hr><center>webserv 'omar hamza'</center>"
        "</body>"
        "</html>";

        response += std::to_string(body.size()) + "\r\n\r\n";
        response += body;
        send(client_fd, response.c_str(), response.size(), 0);
        return;
    }
}



void Client::deleteMethod()
{

}