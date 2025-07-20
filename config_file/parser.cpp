#include "parser.hpp"

void    Parser::parsing(char *fileName)
{
    std::ifstream file(fileName);
    if (file.is_open())
    {
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
                std::cerr << "Expected '{' after 'server'\n";
                return;
            }
            i++;
                Server server;
            if (server.parsServer(tokens, i) == true)
            {
                servers.push_back(server);
            }
        }
        i++;
    }
    servers[0].printf_server();
    std::cout << "======= here =========\n";
    servers[1].printf_server();
    std::cout << "======= here =========\n";
    servers[2].printf_server();
}