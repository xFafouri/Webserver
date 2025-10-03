#include "Location.hpp"
#include "Cgi.hpp"
#include <stdexcept>
#include <map>
#include <utility>

Location::Location()
{
    autoindex = false;
    upload_flag = false;
    cgi_flag = true;
}

void fill_methods(std::map<std::string, bool> &methods)
{
    methods.insert(std::make_pair("GET", false));
    methods.insert(std::make_pair("POST", false));
    methods.insert(std::make_pair("DELETE", false));
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

void Location::print_location()
{
    std::cout << "path ->" << path << std::endl;
    std::cout << "root ->" << root << std::endl;
    for(size_t i = 0; i < index_files.size(); i++)
        std::cout << "index files -> " << index_files[i] << std::endl;
    std::cout << "autoindex -> " << autoindex << std::endl;
    for(size_t i = 0; i < allowed_methods.size(); i++)
        std::cout << "methods -> " << allowed_methods[i] << std::endl;
    std::cout << "upload store -> " << upload_store << std::endl;
    for (std::map<int, std::string>::iterator it = return_value.begin(); it != return_value.end(); it++)
    {
        std::cout << "status code" << it->first << " --> " << "path to redirect" << it->second << std::endl;
    }
    std::cout << "--------------- cgi--------------\n";
    cgi.cgi_print();

}

bool Location::parser_location(size_t &index, std::vector<std::string> &tokens)
{
    if (index >= tokens.size())
        throw std::runtime_error("Unexpected end of tokens at start of location block");
    path = tokens[index];
    if (path == "{" || path == "}" || path == ";" || path.empty() || path[0] != '/')
        throw std::runtime_error("Invalid location path: '" + path + "'. Location path must start with '/'.");
    index += 2;
    for(;index < tokens.size(); index++)
    {
        if (tokens[index] == "root")
        {
            if (index + 2 >= tokens.size())
                throw std::runtime_error("Unexpected end of tokens near 'root' directive");
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
                    break;
                }
                else if (
                    tokens[index] == "root"  || tokens[index] == "autoindex" ||
                    tokens[index] == "cgi_flag" || tokens[index] == "path" || 
                    tokens[index] == "index" || tokens[index] == "upload_store" ||  tokens[index] == "cgi_pass" ||
                     tokens[index] == "return" || tokens[index] == "allowed_methods")
                {
                    throw std::runtime_error("Missing ';' after index_file directive");
                }
                else
                {
                    index_files.push_back(tokens[index]);
                }
                index++;
            }
            if (index >= tokens.size() || tokens[index] != ";")
                throw std::runtime_error("Missing ';' at the end of server_name directive");
        }
        else if (tokens[index] == "autoindex")
        {
            if (index + 2 >= tokens.size())
                throw std::runtime_error("Unexpected end of tokens near 'autoindex' directive");
            if (tokens[index + 2] != ";")
                throw std::runtime_error("error for parsing for autoindex");
            if (tokens[index + 1] == "on")
                autoindex = true;
            else if (tokens[index + 1] == "off")
                autoindex = false;
            else
                throw std::runtime_error("Your autoindex must be 'on' or 'off'");
            index += 2;
        }
        else if (tokens[index] == "upload_flag")
        {
            if (index + 2 >= tokens.size())
                throw std::runtime_error("Unexpected end of tokens near 'upload flag' directive");
            if (tokens[index + 2] != ";")
                throw std::runtime_error("error for parsing for autoindex");
            if (tokens[index + 1] == "on")
                upload_flag = true;
            else if (tokens[index + 1] == "off")
                upload_flag = false;
            else
                throw std::runtime_error("Your upload flag must be 'on' or 'off'");
            index += 2;
        }
        else if (tokens[index] == "upload_store")
        {
            if (index + 2 >= tokens.size())
                throw std::runtime_error("Unexpected end of tokens near 'upload store' directive");
            if (tokens[index + 2] != ";")
                throw std::runtime_error("Missing ';' at the end of upload store directive");
            else
                upload_store = tokens[index + 1];
            index += 2;
        }
        else if (tokens[index] == "allowed_methods")
        {
            index++;
            std::map<std::string, bool> methods;
            fill_methods(methods);
            int checker = 0;
            while (index < tokens.size())
            {
                if (tokens[index] == ";")
                    break;
                if(tokens[index] == "GET")
                {
                    if (methods[tokens[index]] == true)
                        throw std::runtime_error("Error for allowed methods there is more than one 'GET' method");
                    else
                    {
                        allowed_methods.push_back("GET");
                        methods["GET"] = true;
                        checker++;
                    }
                }
                else if (tokens[index] == "POST")
                {
                    if (methods[tokens[index]] == true)
                        throw std::runtime_error("Error for allowed methods there is more than one 'POST' method");
                    else
                    {
                        allowed_methods.push_back("POST");
                        methods["POST"] = true;
                        checker++;
                    }
                }
                else if (tokens[index] == "DELETE")
                {
                    if (methods[tokens[index]] == true)
                        throw std::runtime_error("Error for allowed methods there is more than one 'DELETE' method");
                    else
                    {
                        allowed_methods.push_back("DELETE");
                        methods["DELETE"] = true;
                        checker++;
                    }
                }
                else if (
                    tokens[index] == "root"  || tokens[index] == "autoindex" ||
                    tokens[index] == "cgi_flag" || tokens[index] == "path" || 
                    tokens[index] == "index" || tokens[index] == "upload_store" ||  tokens[index] == "cgi_pass" ||
                     tokens[index] == "return")
                {
                    throw std::runtime_error("Missing ';' at the end of Methods directive");

                }
                else
                    throw std::runtime_error("Invalid method '" + tokens[index] + "' in 'allowed_methods'");
                index++;
            }
        }
        else if (tokens[index] == "return")
        {
            if (index + 3 >= tokens.size())
                throw std::runtime_error("Unexpected end of tokens near 'return' directive");
            if (tokens[index + 3] != ";" )
                throw std::runtime_error("Missing ';' after return directive");
            if (check_is_digit(tokens[index + 1]) == false)
                throw std::runtime_error("Missing HTTP status code in 'return' directive");
            return_value.insert(std::make_pair(ft_atoi(tokens[index + 1].c_str()), tokens[index + 2]));
            index += 3;
        }
        else if (tokens[index] == "cgi_pass")
        {
            index++;
            if (index >= tokens.size() || tokens[index] != "{")
                throw std::runtime_error("Expected '{' after cgi_pass");
            cgi_class temp;
            if (temp.parse_cgi(tokens, index) == true)
                cgi = temp;
        }
        else if (tokens[index] == "cgi_flag")
        {
            if (index + 2 >= tokens.size())
                throw std::runtime_error("Unexpected end of tokens near 'cgi flag' directive");
            if (tokens[index + 2] != ";")
                throw std::runtime_error("error for parsing for autoindex");
            if (tokens[index + 1] == "on")
                cgi_flag = true;
            else if (tokens[index + 1] == "off")
                cgi_flag = false;
            else
                throw std::runtime_error("Your autoindex must be 'ON' or 'OFF'");
            index += 2;   
        }
        else if (tokens[index] == "}")
        {
            return true;
        }
        else
            throw std::runtime_error("Error: Unknown directive in location block.");
    }
    return false; 
}