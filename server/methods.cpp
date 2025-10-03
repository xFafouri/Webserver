#include "server.hpp"
#include <cstddef>
#include <dirent.h>
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

std::string loadErrorPage(const std::string& path) 
{
    std::ifstream file(path.c_str());
    if (!file.is_open()) {
        // fallback default page
        std::stringstream fallback;
        fallback << "<!DOCTYPE html><html><head><title>Error</title></head>"
                 << "<body><h1>Error</h1><p>WebServerhelp/1.1</p></body></html>";
        return fallback.str();
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void Client::sendError(int code)
{
    std::string status_text;
    if (cgi_error_code != 0 && cgi_state == CGI_ERROR)
    {
        code = cgi_error_code;
        cgi_error_code = 0;
    }
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
        body = loadErrorPage(it->second);
    } else {
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

    std::ostringstream headers;
    headers << "HTTP/1.1 " << code << " " << status_text << "\r\n"
            << "Content-Type: text/html\r\n"
            << "Content-Length: " << body.size() << "\r\n"
            << "Connection: close\r\n\r\n";

    response_buffer = headers.str() + body;
}

std::string urlDecode(const std::string &src) {
    std::string result;
    int len = src.length();

    for (int i = 0; i < len; i++) {
        if (src[i] == '%') {
            if (i + 2 < len && std::isxdigit(src[i+1]) && std::isxdigit(src[i+2])) {
                char hex[3] = { src[i+1], src[i+2], '\0' };
                int value = strtol(hex, NULL, 16);

                // Prevent directory traversal: ignore '/' decoding
                if (value == '/') {
                    result += "%2F"; // keep encoded
                } else {
                    result += static_cast<char>(value);
                }
                i += 2;
            }
        } else if (src[i] == '+') {
            result += ' ';
        } else {
            result += src[i];
        }
    }
    return result;
}

void Client::getMethod()
{
    std::string decode;
    decode = urlDecode(Hreq.uri);
    const Location* matchedLocation = NULL;
    decode = normalize_path(decode);
    //todo segmentation
    for (size_t i = 0; i < config.locations.size(); ++i)
    {
        if (decode.find(config.locations[i].path) == 0)
        {
            if (!matchedLocation || config.locations[i].path.length() > matchedLocation->path.length())
            {
                matchedLocation = &config.locations[i];
            }
        }
    }

    if (!matchedLocation->return_value.empty())
    {
        int status_code = matchedLocation->return_value.begin()->first;
        std::string redirect_url = matchedLocation->return_value.begin()->second;

        response_buffer = "HTTP/1.1 " + to_string98(status_code) + " ";

        if (status_code == 301)
            response_buffer += "Moved Permanently\r\n";
        else if (status_code == 302)
            response_buffer += "Found\r\n";
        else if (status_code == 307)
            response_buffer += "Temporary Redirect\r\n";
        else if (status_code == 308)
            response_buffer += "Permanent Redirect\r\n";
        else
            response_buffer += "Redirect\r\n"; 

        response_buffer += "Location: " + redirect_url + "\r\n";
        response_buffer += "Content-Length: 0\r\n";
        response_buffer += "Connection: close\r\n\r\n";

        return;
    }
    
    // std::cout << "matchedlocation == " << matchedLocation->path << std::endl;
    if (!matchedLocation)
    {
        sendError(404);
        return;
    }

    std::string location_path = matchedLocation->path;
    // std::cout << "location path == " << location_path << std::endl;
    std::string relative_uri = decode.substr(location_path.length());
    // std::cout << "relative path ==" << relative_uri << std::endl;

    if (!relative_uri.empty() && relative_uri[0] == '/')
        relative_uri.erase(0, 1);

    std::string full_dir_path = matchedLocation->root;
    if (!full_dir_path.empty() && full_dir_path[full_dir_path.size() - 1] == '/')
        full_dir_path.erase(full_dir_path.size() - 1);
    // std::cout << "full_dir_path check / == " << full_dir_path.back() << std::endl;
    // std::cout << "full dir path == " << full_dir_path << std::endl;
    while (!full_dir_path.empty() && full_dir_path[full_dir_path.size() - 1] == '/')
        full_dir_path.erase(full_dir_path.size() - 1);
// avoid double slashes

    std::string full_path = full_dir_path;
    if (!relative_uri.empty())
        full_path += "/" + relative_uri;
    // std::cout << "full path === " << full_path << std::endl;
    struct stat st;
    if (stat(full_path.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
    {
        // std::cout << "is a directory " << std::endl;
        if (!decode.empty() && decode[decode.size() - 1] != '/')
        {
            std::string redirect_uri = decode + "/";
            response_buffer = "HTTP/1.1 301 Moved Permanently\r\n";
            response_buffer += "Location: " + redirect_uri + "\r\n";
            response_buffer += "Content-Length: 0\r\n";
            response_buffer += "Connection: close\r\n\r\n";
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
        
        // std::cout << "index path --->>>>> " << index_path << std::endl;
        if (foundIndex)
        {
            std::ifstream file(index_path.c_str());
            if (!file)
            {
                response_buffer = "HTTP/1.1 500 Internal Server Error\r\nContent-Length: 0\r\n\r\n";
                return;
            }

            std::ostringstream ss;
            ss << file.rdbuf();
            std::string body = ss.str();
            std::string content_type = ft_content_type(index_path);
            response_buffer = "HTTP/1.1 200 OK\r\n";
            response_buffer += "Content-Type: "+ content_type + "\r\n";
            response_buffer += "Content-Length: " + to_string98(body.size()) + "\r\n";
            response_buffer += "Connection: close\r\n\r\n";

            response_buffer += body;
            return;
        }
        else if (matchedLocation->autoindex)
        {
            // std::cout << "full path dir ====> " << full_dir_path <<  std::endl;
            DIR *dir = opendir(full_path.c_str());
            if (!dir)
            {
                perror("opendir");
                sendError(403);
                return;
            }
        
            struct dirent *entry;
            response_buffer = "HTTP/1.1 200 OK\r\n";
            response_buffer += "Content-Type: text/html\r\n";
            response_buffer += "Connection: close\r\n\r\n";
            response_buffer += "<!DOCTYPE html><html><head>";
            response_buffer += "<meta charset=\"UTF-8\">";
            response_buffer += "<title>Index of " + decode + "</title>";
            response_buffer += "<style>";
            response_buffer += "body { font-family: Arial, sans-serif; background:#f9f9f9; margin:20px; }";
            response_buffer += "h1 { font-size:22px; margin-bottom:15px; }";
            response_buffer += "ul { list-style:none; padding:0; }";
            response_buffer += "li { margin:6px 0; }";
            response_buffer += "a { text-decoration:none; color:#007BFF; font-weight:bold; }";
            response_buffer += "a:hover { color:#0056b3; text-decoration:underline; }";
            response_buffer += "</style></head><body>";
            while ((entry = readdir(dir)) != NULL)
            {
                std::string name = entry->d_name;
                std::string entry_path = full_path + "/" + name;
                struct stat st;
                if (stat(entry_path.c_str(), &st) == 0 && S_ISDIR(st.st_mode))
                {
                    name += "/";
                }
                response_buffer += "<li><a href=\"" + decode;
                if (decode[decode.size() - 1] != '/')
                    response_buffer += "/";
                response_buffer += name + "\">" + name + "</a></li>";
            }
            response_buffer += "</ul></body></html>";
            closedir(dir);
            return;
        }

        else
        {
            sendError(403);
            return;
        }
    }
    else if (stat(full_path.c_str(), &st) == 0 && S_ISREG(st.st_mode))
    {
        std::ifstream file(full_path.c_str());
        if (!file)
        {
            sendError(500);
            return;
        }

        std::ostringstream ss;
        // std::cout << "here is ---> " << full_path << std::endl;
        ss << file.rdbuf();

        std::string body = ss.str();
        std::string content_type = ft_content_type(full_path);
        response_buffer = "HTTP/1.1 200 OK\r\n";
        response_buffer += "Content-Type: " + content_type + "\r\n";
        response_buffer += "Content-Length: " + to_string98(body.size()) + "\r\n";
        response_buffer += "Connection: close\r\n\r\n";
        response_buffer += body;
        return;
    }
    else
    {
        sendError(404);
        return;
    }
}

void Client::deleteMethod()
{
    std::string decode = urlDecode(Hreq.uri);
    decode = normalize_path(decode);

    const Location* matchedLocation = NULL;
    for (size_t i = 0; i < config.locations.size(); ++i)
    {
        if (decode.find(config.locations[i].path) == 0)
        {
            if (!matchedLocation || config.locations[i].path.length() > matchedLocation->path.length())
                matchedLocation = &config.locations[i];
        }
    }

    if (!matchedLocation)
    {
        sendError(404);
        return;
    }


    std::string relative_uri = decode.substr(matchedLocation->path.length());
    if (!relative_uri.empty() && relative_uri[0] == '/')
        relative_uri.erase(0,1);

    std::string full_path = matchedLocation->root;
    while (!full_path.empty() && full_path[full_path.size() - 1] == '/')
        full_path.erase(full_path.size() - 1);

    if (!relative_uri.empty())
        full_path += "/" + relative_uri;

    // std::cout << "DELETE ==> full path = " << full_path << std::endl;

    std::string uploadDir = matchedLocation->upload_store;
    if (!uploadDir.empty())
    {
        uploadDir = normalize_path(uploadDir);
        if (!uploadDir.empty() && uploadDir[uploadDir.size() - 1] != '/')
            uploadDir += '/';

        std::string normalizedFullPath = normalize_path(full_path);

        if (normalizedFullPath.find(uploadDir) != 0)
        {
            sendError(403);
            return;
        }
    }
    else
    {
        sendError(403);
        return;
    }
    
    struct stat st;
    if (stat(full_path.c_str(), &st) != 0)
    {
        sendError(404);
        return;
    }
    
    if (S_ISDIR(st.st_mode))
    {
        sendError(403);
        return;
    }

    if (remove(full_path.c_str()) != 0)
    {
        perror("delete");
        sendError(500);
        return;
    }
    
    response_buffer = "HTTP/1.1 204 No Content\r\n";
    response_buffer += "Content-Length: 0\r\n";
    response_buffer += "Connection: close\r\n\r\n";
}
