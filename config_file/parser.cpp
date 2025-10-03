#include "parser.hpp"
#include <stdexcept>

void Parser::check_hosting_port(std::vector<ServerCo> servers)
{
    for(size_t i = 0; i < servers.size() ; i++)
    {
        for(size_t j = i + 1; j < servers.size(); j++)
        {
            if ((servers[i].host == servers[j].host) && (servers[i].listen == servers[j].listen))
            {
                if (servers[i].server_names.empty() && servers[j].server_names.empty())
                {
                    throw std::runtime_error("Configuration Error: Duplicate server definition detected.\n"
                        "  - server_name: 'empty' \n"
                        "  - listen: " + to_string(servers[i].listen) + "\n"
                        "  - host: " + servers[i].host + "\n\n"
                        "Each server must have a unique combination of server_name, listen, and host.");
                }
                else if ((!servers[i].server_names.empty() && !servers[j].server_names.empty()) && (servers[i].server_names[0] == servers[j].server_names[0]))
                {
                    throw std::runtime_error("Configuration Error: Duplicate server definition detected.\n"
                        "  - server_name: " + servers[i].server_names[0] + "\n"
                        "  - listen: " + to_string(servers[i].listen) + "\n"
                        "  - host: " + servers[i].host + "\n\n"
                        "Each server must have a unique combination of server_name, listen, and host.");
                }
            }
        }
    }
}

void    Parser::parsing(std::string fileName)
{
    std::ifstream file(fileName.c_str());
    if (file.is_open())
    {
        if (file.peek() == std::ifstream::traits_type::eof())
        {
            file.close();
            throw std::runtime_error("Error: Config file is empty!");
        }
        std::string line;
        while (std::getline(file, line))
        {
            std::string token;
            for (size_t i = 0; i < line.size(); i++)
            {
                char c = line[i];
                if (isspace(c))
                {
                    if (!token.empty())
                    { 
                        tokens.push_back(token);
                        token.clear();
                    }
                }
                else if (c == '{' || c == '}' || c == ';')
                {
                    if (!token.empty())
                    {
                        tokens.push_back(token);
                        token.clear();
                    }
                    tokens.push_back(std::string(1 ,c));
                }
                else
                    token += c;
            }
            if (!token.empty())
                tokens.push_back(token);

        }
        if (tokens.empty())
            throw std::runtime_error("Error: Config file is empty!11");
        file.close();
        parse();
    }
    else
    {
        throw std::runtime_error("Failed to open file.");
    }
}

void    Parser::parse()
{
    size_t i = 0;

    while (i < tokens.size())
    {
        if (tokens[i] == "server")
        {
            i++;
            if (tokens[i] != "{")
            {
                throw std::runtime_error("Expected '{' after 'server'");
            }
            i++;
                ServerCo server;
            if (server.parsServer(tokens, i) == true)
            {
                servers.push_back(server);
            }
            else 
                throw std::runtime_error("Expected '}' at the end of the server block");
        }
        else
            throw std::runtime_error("Error : for block of servers");
        i++;
    }
        check_hosting_port(servers);
}