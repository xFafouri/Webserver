#include "parser.hpp"
#include <stdexcept>

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
                    tokens.push_back(std::string(1 ,c)); // push { or } or ;
                }
                else
                    token += c;
            }
            if (!token.empty())
                tokens.push_back(token);

        }
        // for(size_t i = 0; i < tokens.size(); i++)
        // {
        //     std::cout << tokens[i] << std::endl;
        // }
        file.close();
        parse();
    }
    else
    {
        std::cout << "Failed to open file." << std::endl;
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
    // for(size_t i = 0;i < servers.size(); i++)
    // {
    //     std::cout << "Server number ["  << i + 1 << "]"<< std::endl;
    //     servers[i].printf_server();
    // }
}