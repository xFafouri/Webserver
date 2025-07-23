#include "Server.hpp"
#include "Location.hpp"
#include <stdexcept>

Server::Server()
{
    host = "";
    listen = -1;
    client_max_body_size = 0;
}

void Server::validate() {
    if (this->listen == -1)
        throw std::runtime_error("Missing 'listen' directive");
    if (this->host.empty())
        throw std::runtime_error("Missing 'host' directive");
    if (this->locations.empty())
        throw std::runtime_error("At least one location block required");

    for (size_t i = 0; i < locations.size(); i++) {
        if (locations[i].root.empty())
            throw std::runtime_error("Location block missing 'root' directive");
    }
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

static long long ft_atoi(const char *str)
{
    int	i;
	long long	sign;
	long long	factorial;

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

    for(size_t i = 0; i < locations.size(); i++)
    {
        std::cout << "..... location .........\n";
        locations[i].print_location();
    }
}

bool    Server::parsServer(std::vector<std::string> &tokens, size_t &index)
{
    bool listen_seen = false;
    bool host_seen = false;
    for(;index < tokens.size(); index++)
    {
        // std::cout << tokens[index] << std::endl;
        if (tokens[index] == "listen")
        {
            if (tokens[index + 2] != ";")
                throw std::runtime_error("Missing ';' at the end of listen directive");
            if (listen_seen)
                    throw std::runtime_error("Duplicate 'listen' directive found.");
            else
            {
                if (check_is_digit(tokens[index + 1]) == true && tokens[index + 2] == ";")
                        listen = ft_atoi(tokens[index + 1].c_str());
                else
                    throw std::runtime_error("Error for parsing1");
                listen_seen = true;
            }
            index += 2;

        }
        else if (tokens[index] == "host")
        {
            if (tokens[index + 2] != ";")
                throw std::runtime_error("Missing ';' at the end of host directive");
            if (host_seen)
                    throw std::runtime_error("Duplicate 'host' directive found.");
            else
            {
                host = tokens[index + 1];
                host_seen = true;
            }
            index += 2;
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
        }
        else if (tokens[index] == "error_page")
        {
            if (tokens[index + 3] != ";")
                throw std::runtime_error("Missing ';' at the end of Error page directive");
            if (check_is_digit(tokens[index + 1]) == true)
                error_pages.insert(std::make_pair(ft_atoi(tokens[index + 1].c_str()), tokens[index + 2]));
            else
                throw std::runtime_error("ERROR Parsing for Error_page directive");
            index += 3;
        }
        else if (tokens[index] == "client_max_body_size")
        {
            if (tokens[index + 2] != ";")
                throw std::runtime_error("Missing ';' at the end of client max body size directive");
            index++;
            if (check_is_body_size(tokens[index]))
            {
                client_max_body_size = ft_atoi(tokens[index].c_str());
                client_max_body_size *= 1048576;
            }
            else 
            {
                throw std::runtime_error("Invalid format for client_max_body_size. Use format like '10M'.");
            }
            index += 1;
        }
        else if (tokens[index] == "location")
        {
            Location location;
            index++;
            if (tokens[index] == "{")
                throw std::runtime_error("Invalid or missing path after 'location'.");
            if (tokens[index + 1] != "{")
                throw std::runtime_error("error for parsing your location must has a '{' ");
            if (location.parser_location(index, tokens) == true)
                locations.push_back(location);
            else
                throw std::runtime_error("Expected '}' at the end of location block");
        }
        else if (tokens[index] == "}" && (tokens[index + 1] == "server" || index + 1 == tokens.size()))
        {
            validate();
            return true;
        }
        else
            throw std::runtime_error("Error: Unknown directive in server block.");
    }
    return false;
}
