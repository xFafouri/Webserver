#include "Location.hpp"

Location::Location()
{
    client_max_body_size = 0;
    autoindex = false;
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

bool Location::parser_location(size_t &index, std::vector<std::string> tokens)
{
    path = tokens[index];
    index += 2;
    for(;index < tokens.size(); index++)
    {
        if (tokens[index] == "root")
        {
            if (tokens[index + 2] != ";")
                throw std::runtime_error("error for parsing");
            root = tokens[index + 1];
            index += 2;
        }
        else if (tokens[index] == "index")
        {
            index++;
            while (index < tokens.size())
            {
                if (tokens[index] == ";")
                {
                    // End of index directive
                    break;
                }
                else if (
                    tokens[index] == "root" || tokens[index] == "methods" || 
                    tokens[index] == "client_max_body_size" || tokens[index] == "autoindex" || 
                    tokens[index] == "allowed_methods" || tokens[index] == "path" || 
                    tokens[index] == "index_flag" || tokens[index] == "index")
                {
                    // We encountered a valid directive keyword without seeing a ';' first
                    throw std::runtime_error("Missing ';' after index_file directive");
                }
                else
                {
                    index_files.push_back(tokens[index]);
                }
                index++;
            }
            if (tokens[index] != ";")
                throw std::runtime_error("Missing ';' at the end of server_name directive");
        }
        else if (tokens[index] == "client_max_body_size")
        {
            if (tokens[index + 2] != ";")
                throw std::runtime_error("Error for parsing");
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
    }
    return true; 
}