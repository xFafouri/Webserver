#include "Server.hpp"
#include "Location.hpp"
#include <stdexcept>

ServerCo::ServerCo()
{
    host = "";
    listen = -1;
    client_max_body_size = 0;
}

void ServerCo::validate() {
    if (this->listen == -1)
        throw std::runtime_error("Missing 'listen' directive");
    if (this->host.empty())
        throw std::runtime_error("Missing 'host' directive");
    if (this->locations.empty())
        throw std::runtime_error("At least one location block required");

    for (size_t i = 0; i < locations.size(); i++) {
        if (locations[i].root.empty() && locations[i].return_value.empty())
            throw std::runtime_error("Location block missing 'root' directive");
    }
    for (size_t i = 0; i < locations.size(); i++)
    {
        for (size_t j = i + 1; j < locations.size(); j++)
        {
            if (locations[i].path == locations[j].path)
                throw std::runtime_error("Configuration Error: A path cannot be assigned to more than one location block.");
        }
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

std::vector<std::string> split(const std::string& str, char delim) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream stream(str);
    
    while (getline(stream, token, delim)) {
        if (!token.empty()) {
            tokens.push_back(token);
        }
    }
    return tokens;
}


static bool check_host(const std::string& host_p)
{
    const char *str = host_p.c_str();
    int check = 0;
    if(strcmp("dump-ubuntu-benguerir",str) == 0 || strcmp("localhost",str) == 0)
        return true;
    else
    {
        if(str[0] == '.')
            return false;
        for (size_t i = 0; i < strlen(str); i++)
        {
            if(str[i] == '.' && check == 0)
                check = 1;
            else if(str[i] == '.' && check == 1)
                return false;
            else if(str[i] != '.')
                check = 0;
            if(!isdigit(str[i]) && str[i] != '.')
                return false;
        }
        if(str[strlen(str) - 1] == '.' || split(host_p,'.').size() != 4)
            return false;
    }
    return true;
}

bool    ServerCo::parsServer(std::vector<std::string> &tokens, size_t &index)
{
    bool listen_seen = false;
    bool host_seen = false;
    for(;index < tokens.size(); index++)
    {
        // std::cout << tokens[index] << std::endl;
        if (tokens[index] == "listen")
        {
            if (index + 2 >= tokens.size())
                throw std::runtime_error("Unexpected end of tokens near 'listen' directive");
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
                if (listen < 1 || listen > 65535)
                    throw std::runtime_error("Invalid port: must be between 1 and 65535");
                listen_seen = true;
            }
            index += 2;
        }
        else if (tokens[index] == "host")
        {
            if (index + 2 >= tokens.size())
                throw std::runtime_error("Unexpected end of tokens near 'host' directive");
            if (tokens[index + 2] != ";")
                throw std::runtime_error("Missing ';' at the end of host directive");
            if (host_seen)
                    throw std::runtime_error("Duplicate 'host' directive found.");
            else if  (check_host(tokens[index + 1]) == false)
            {
                    throw std::runtime_error("Error: Invalid host. Host must be 'localhost', 'dump-ubuntu-benguerir', or a valid IP address."); 
            }
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
        
            if (index >= tokens.size() || tokens[index] != ";")
                throw std::runtime_error("Missing ';' at the end of server_name directive");
        }
        else if (tokens[index] == "error_page")
        {
            if (index + 3 >= tokens.size())
                throw std::runtime_error("Unexpected end of tokens near 'error_page' directive");
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
            if (index + 2 >= tokens.size())
                throw std::runtime_error("Unexpected end of tokens near 'client_max_body_size' directive");
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
            if (index >= tokens.size())
                throw std::runtime_error("Unexpected end of tokens near 'location' directive");
            if (tokens[index] == "{")
                throw std::runtime_error("Invalid or missing path after 'location'.");
            if (index + 1 >= tokens.size())
                throw std::runtime_error("Unexpected end of tokens near 'location' directive");
            if (tokens[index + 1] != "{")
                throw std::runtime_error("error for parsing your location must has a '{' ");
            if (location.parser_location(index, tokens) == true)
            {
                locations.push_back(location);
            }
            else
                throw std::runtime_error("Expected '}' at the end of location block");
        }
        else if (tokens[index] == "}" && ( (index + 1 < tokens.size() && tokens[index + 1] == "server") || index + 1 == tokens.size()))
        {
            validate();
            return true;
        }
        else
            throw std::runtime_error("Error: Unknown directive in server block.");
    }
    return false;
}
