#include "Server.hpp"

Server::Server()
{
    host = "";
    listen = 80;
    client_max_body_size = 0;
}

bool    check_is_digit(std::string string)
{
    for (size_t i = 0; i < string.size(); i++)
    {
        if (!isdigit(string[i]))
            return false;
    }
    return true;
}

bool    Server::parsServer(std::vector<std::string> tokens, size_t &index)
{
    for(;index < tokens.size(); index++)
    {
        if (tokens[index] == "listen")
        {
            if (check_is_digit(tokens[index + 1]) == true && tokens[index + 2] == ";")
                listen = atoi(tokens[index].c_str());
            else
                throw std::runtime_error("error for parsing");
            index += 2;
        }
        else if (tokens[index] == "host")
        {
            if (tokens[index + 2] == ";")
            {
                host = tokens[index + 1];
                index += 2;
            }
            else
                throw std::runtime_error("error for parsing");
        }
        else if (tokens[index] == "server_name")
        {
            index++; // Move to the first name
            while (index < tokens.size())
            {
                if (tokens[index] == ";")
                {
                    // End of server_name directive
                    break;
                }
                else if (
                    tokens[index] == "listen" || tokens[index] == "host" || 
                    tokens[index] == "server_name" || tokens[index] == "error_page" || 
                    tokens[index] == "location" || tokens[index] == "root" || 
                    tokens[index] == "limit_except" || tokens[index] == "client_max_body_size"
                )
                {
                    // We encountered a valid directive keyword without seeing a ';' first
                    throw std::runtime_error("Missing ';' after server_name directive");
                }
                else
                {
                    server_names.push_back(tokens[index]);
                }
            
                index++;
            }
        
            // After loop, we must be at the semicolon
            if (tokens[index] != ";")
                throw std::runtime_error("Missing ';' at the end of server_name directive");
        
            index++; // Move past the semicolon
        }
        else if (tokens[index] == "error_page")
        {
            index++;
            std::vector<int> codes;
            size_t pos = index;
            while (index < tokens.size() && tokens.back() != ";")
                pos++;
            if (pos == tokens.size())
                throw  std::runtime_error("Missing ';' at end of error_page directive");
            std::string testing = tokens[pos];
            if (testing == )
        }
    }
}



