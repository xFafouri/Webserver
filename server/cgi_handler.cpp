
#include "server.hpp"
#include <cstddef>
#include <sys/types.h>

bool Client::is_cgi_request() 
{
    // std::map<std::string , std::string> map_ext;
    ///scripts/test.py?name=hamza
    std::cout << "here "  << Hreq.uri << std::endl;
    size_t dot_pos = Hreq.uri.rfind('.');
    if (dot_pos == std::string::npos)
        return false;
    size_t qs = Hreq.uri.find('?');
    if (qs != std::string::npos)
    {
        if (Hreq.method == "GET")
        {
            std::cout << "GET script_file " << std::endl;
            script_file = "." + Hreq.uri.substr(0,qs);
        }
    }
    else
    {
        std::cout << "without Query" << std::endl;
        script_file = "." + Hreq.uri;
    } 
    std::cout << "script_file = " << script_file << std::endl;
    //.py?name=hamza
    std::string ext = Hreq.uri.substr(dot_pos, qs - dot_pos);

    std::cout << "extension = " << ext << std::endl;
    
    int i = 0;
    cgi_bin = false;
    std::cout << "locations size = " << config.locations.size() << std::endl;
    for(; i < config.locations.size() ;i++)
    {
        std::cout << "ROOT = " << config.locations[i].path << std::endl;
        if (config.locations[i].path == "/cgi-bin")
        {
            cgi_bin = true;
            break;
        }
    }
    if (cgi_bin == false || ext.empty())
    {
        std::cout << "cgi_bin false" << std::endl;
        return false;
    }
    std::cout << "mo7alch" << std::endl;
    location_idx = i;
    extension = ext;

    // if (ext == config.locations[i].cgi.php)
    // {
    // }
    std::cout << "mo7alch1" << std::endl;

    map_ext.insert(std::make_pair(".php", config.locations[i].cgi.php));
    map_ext.insert(std::make_pair(".py", config.locations[i].cgi.py));
    map_ext.insert(std::make_pair(".pl", config.locations[i].cgi.pl));
    std::cout << "mo7alch2" << std::endl;
    
    // std::cout << "EXTENSION = " << config.locations[i].cgi.php << std::endl;
    return (ext == ".py" || ext == ".php" || ext == ".pl");
}



void    Client::run_cgi()
{
    std::string query_string;
    env_map.insert(std::make_pair("REQUEST_METHOD", Hreq.method));
    // env_map.insert(std::make_pair("SCRIPT_FILENAME", map_ext[extension]));
    // env_map.insert(std::make_pair("SCRIPT_FILENAME", map_ext[extension]));
    std::string script_path = "./server/test.py";
    env_map.insert(std::make_pair("SCRIPT_FILENAME", script_file));

    std::cout << "extesnion = " << map_ext[extension] << std::endl;
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
        std::cout << "Content_type = " << Hreq.content_type << std::endl;
        env_map.insert(std::make_pair("CONTENT_TYPE", Hreq.content_type));
        env_map.insert(std::make_pair("CONTENT_LENGTH", std::to_string(Hreq.content_length)));
        std::cout << "content length = " << Hreq.content_length << std::endl;

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
        std::cout << "env = " << envp[i] << std::endl;
        i++;
    }
    char *argv[] = {
    (char*)map_ext[extension].c_str(),     
    (char*)script_file.c_str(),        
    NULL
    };
    // char *argv[] = {(char*)map_ext[extension].c_str(), NULL };

    std::cout << "fork" << std::endl;
    int in_pipe[2];
    int out_pipe[2];
    pipe(in_pipe);
    pipe(out_pipe);

    int pid = fork();

    if (pid == 0)
    {
        dup2(in_pipe[0], STDIN_FILENO);
        dup2(out_pipe[1], STDOUT_FILENO);

        close(in_pipe[1]);
        close(out_pipe[0]);

        execve(argv[0], argv, envp);
    }

    close(in_pipe[0]);
    close(out_pipe[1]);
    
    if (Hreq.method == "POST") 
    {
        write(in_pipe[1], Hreq.body._body.c_str(), Hreq.body._body.size());
    }
    close(in_pipe[1]);
    
    char buffer[8192];
    ssize_t n;
    std::string cgi_output;
    
    while ((n = read(out_pipe[0], buffer, sizeof(buffer))) > 0)
        cgi_output.append(buffer, n);

    close(out_pipe[0]);
    waitpid(pid, NULL, 0);
    
    std::stringstream response;
    size_t header_end = cgi_output.find("\r\n\r\n");
    if (header_end == std::string::npos) 
    {
        header_end = cgi_output.find("\n\n"); 
    }

    std::string headers;
    std::string body;
    if (header_end != std::string::npos)
    {
        headers = cgi_output.substr(0, header_end);
        body = cgi_output.substr(header_end + 2);
    } 
    else 
    {
        body = cgi_output;
    }

    std::string content_type = "text/html";
    std::istringstream hstream(headers);
    std::string line;
    while (std::getline(hstream, line)) 
    {
        size_t n = line.find("Content-Type:");
        if (line.find("Content-Type:") != std::string::npos) 
        {
            content_type = line.substr(n + 13);
            while (!content_type.empty() && (content_type[0] == ' ' || content_type[0] == '\t'))
                content_type.erase(0, 1);
            std::cout << "content_type1 = " << content_type << std::endl;
        }
    }

    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: " << content_type << "\r\n";
    response << "Content-Length: " << cgi_output.size() << "\r\n";
    response << "\r\n";
    response << cgi_output;
    std::cout << "headers = " << headers << std::endl;
    std::cout << "body = " << body << std::endl;
    
    response_buffer = response.str();
    // std::cout << "response_buffer  = " << response_buffer << std::endl;
}
