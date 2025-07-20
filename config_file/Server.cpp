#include "Server.hpp"
#include "Location.hpp"
#include <stdexcept>

Server::Server()
{
    host = "";
    listen = 80;
    client_max_body_size = 0;
}

static bool    check_is_digit(std::string string)
{
    for (size_t i = 0; i < string.size(); i++)
    {
        if (!isdigit(string[i]))
            return false;
    }
    return true;
}

static bool    check_is_body_size(std::string string)
{
    for (size_t i = 0; i < string.size(); i++)
    {
        if (!isdigit(string[i]) && string[i + 1] != '\0')
            return false;
        else if (!isdigit(string[i]) && string[i + 1] == '\0')
        {
            // std::cout << "here\n";
            if (string[i] == 'M')
                return true;
            else
                throw std::runtime_error("your client max body size must be on MEGABYTES");
        }
        else if (isdigit(string[i]) && string[i + 1] == '\0')
            throw std::runtime_error("your must to add 'M' to specific it's a MEGABYTES");

    }
    return true;
}

static long ft_atoi(const char *str)
{
    int	i;
	long	sign;
	long	factorial;

	i = 0;
	sign = 1;
	while (str[i] == ' ' || (str[i] >= 9 && str[i] <= 13))
		i++;
	if (str[i] == '-' || str[i] == '+')
	{
		if (str[i] == '-')
			sign = -sign;
		i++;
	}
	factorial = 0;
	while (str[i] >= '0' && str[i] <= '9')
	{
		factorial = (10 * factorial) + str[i] - '0';
		i++;
	}
	return (factorial * sign);
}

void Server::printf_server()
{
    // std::cout << "here\n";
    std::cout << "listen is --> " << listen <<std::endl;
    std::cout << "host is -->" << host <<std::endl;
    for (size_t i = 0; i < server_names.size(); i++)
    {
        std::cout << "server name is  "<< i + 1 << "-->" << server_names[i] << std::endl;
    }
    std::cout << "client max body size is --> " << client_max_body_size <<std::endl;
    for (std::map<int, std::string>::iterator it = error_pages.begin(); it != error_pages.end(); it++)
    {
        std::cout << it->first << " --> " << it->second << std::endl;
    }

}

bool    Server::parsServer(std::vector<std::string> tokens, size_t &index)
{
    for(;index < tokens.size(); index++)
    {
        // std::cout << tokens[index] << std::endl;
        if (tokens[index] == "listen")
        {
            if (check_is_digit(tokens[index + 1]) == true && tokens[index + 2] == ";")
            {
                listen = ft_atoi(tokens[index + 1].c_str());
            }
            else
                throw std::runtime_error("Error for parsing1");
            index += 2;

        }
        else if (tokens[index] == "host")
        {
            if (tokens[index + 2] == ";")
            {
                // std::cout << "host ===> " << host << std::endl;
                host = tokens[index + 1];
                index += 2;
            }
            else
                throw std::runtime_error("error for parsing2");
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
        
            // index++; // Move past the semicolon
        }
        else if (tokens[index] == "error_page")
        {
            index++;
            std::vector<int> codes;
            std::string uri;

            // Loop until we find the ";" token
            while (index < tokens.size() && tokens[index] != ";")
            {
                if (isdigit(tokens[index][0])) // must to check all the string
                {
                    codes.push_back(std::atoi(tokens[index].c_str()));
                }
                else
                {
                    uri = tokens[index]; // This should be the URI string
                }
                index++;
            }

            if (index == tokens.size() || tokens[index] != ";")
                throw std::runtime_error("Missing ';' at end of error_page directive");
            
            // Save error pages into your map<int, string>
            for (size_t i = 0; i < codes.size(); ++i)
                error_pages[codes[i]] = uri;
            
            // index++; // Move past the semicolon
        }
        else if (tokens[index] == "client_max_body_size")
        {
            if (tokens[index + 2] != ";")
                throw std::runtime_error("Error for parsing3");
            index++;
            if (check_is_body_size(tokens[index]))
            {
                client_max_body_size = ft_atoi(tokens[index].c_str());
                client_max_body_size *= 1048576;
                // std::cout << client_max_body_size << " <--here\n";
            }
            else 
            {
                throw std::runtime_error("client max body size is not an integer");
            }
            index += 1;
            // std::cout << "here\n";
        }
        else if (tokens[index] == "location")
        {
            Location location;
            index++;
            if (tokens[index + 1] != "{")
                throw std::runtime_error("error for parsing your location must has a '{' ");
            if (location.parser_location(index, tokens) == true)
                locations[tokens[index]] = location;
        }
        else if (tokens[index] == "}")
            return true;
        else
            throw std::runtime_error("Error for parsing4");
    }
    return false;
}
