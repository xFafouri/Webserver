
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
    static const std::vector<std::string> cgi_exts = {".py", ".pl", ".cgi"};
    
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
    const Location* matchedLocation = nullptr;

    
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
    // std::cout << "@@@@@@@HERE00@@@@@" << std::endl;
    if (!matchedLocation)
        return false;
    if (!matchedLocation->cgi_flag)
        return false;

    // 
    if (Hreq.uri == matchedLocation->path || Hreq.uri == matchedLocation->path + "/") 
    {
        for (size_t i = 0; i < matchedLocation->index_files.size(); ++i) 
        {
            std::string index_path = matchedLocation->root + "/" + matchedLocation->index_files[i];
            if (access(index_path.c_str(), F_OK) == 0) 
            {
                script_file = index_path;
                std::cout << script_file << std::endl;
                // std::cout << "@@@@@@@HERE11@@@@@" << std::endl;
                extension = ft_content_type_cgi(script_file);
                cgi_bin = true;
                map_ext[".php"] = matchedLocation->cgi.php;
                map_ext[".py"]  = matchedLocation->cgi.py;
                map_ext[".pl"]  = matchedLocation->cgi.pl;
                return true;
            }
        }
        // std::cout << "@@@@@@@HERE22@@@@@" << std::endl;
        return false; 
    }

    //cgi-bin/whatever.py
    {
        std::string relative_uri = Hreq.uri.substr(matchedLocation->path.length());
        if (!relative_uri.empty() && relative_uri[0] == '/')
            relative_uri.erase(0, 1);

        script_file = matchedLocation->root + "/" + relative_uri;

        std::cout << "here" << std::endl;
        size_t dot_pos = script_file.rfind('.');
        std::cout << "pos = " << dot_pos << std::endl;
        if (dot_pos == std::string::npos)
            return false;

        extension = script_file.substr(dot_pos);

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

void    Client::run_cgi()
{
    std::string query_string;
    env_map.insert(std::make_pair("REQUEST_METHOD", Hreq.method));
    // env_map.insert(std::make_pair("SCRIPT_FILENAME", map_ext[extension]));
    // env_map.insert(std::make_pair("SCRIPT_FILENAME", map_ext[extension]));
    // std::string script_file = "test.py";
    // script_file = "./www/main/" + script_file;
    // std::cout << "script_file = " << script_file << std::endl;
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
    std::cout << "map[ext] = " <<  map_ext[extension] << std::endl;
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
    // pipe(fd);

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
    size_t sep_len = 4;
    if (header_end == std::string::npos) 
    {
        header_end = cgi_output.find("\n\n");
        sep_len = 2;
    }

    std::string headers;
    std::string body;
    if (header_end != std::string::npos)
    {
        headers = cgi_output.substr(0, header_end);
        std::cout << "header_end" << header_end << std::endl;
        body = cgi_output.substr(header_end + sep_len); 
        // std::cout << "body" << body << std::endl;
    } 
    else 
    {
        // std::cout << "123" << std::endl;
        body = cgi_output;
    }

    if (body.empty())
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
    // content_type = "binary/octet-stream";
    response << "HTTP/1.1 200 OK\r\n";
    response << "Content-Type: " << content_type << "\r\n";
    // std::cout << "content-type = " <<  content_type << std::endl;
    response << "Content-Length: " << body.size() << "\r\n";
        // std::cout << "headers = " << headers << std::endl;

    // response << headers << "\r\n";
    response << "\r\n";
    response << body;
    // std::cout  << "body = " << body << std::endl;
    response_buffer = response.str();
    // std::cout << "response_buffer = " <<  response_buffer << std::endl; 

}
