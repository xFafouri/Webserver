#include "Cgi.hpp"

void cgi_class::cgi_print()
{
    std::cout << "php --> " << php << std::endl;
    std::cout << "py --> " << py << std::endl;
    std::cout << "pl --> " << pl << std::endl;
}

bool cgi_class::parse_cgi(std::vector<std::string> &tokens, size_t &index)
{
    index++;
    while (index < tokens.size())
    {
        // std::cout << tokens[index] << std::endl;
        if (tokens[index] == ".php")
        {
            if (tokens[index + 2] != ";")
                throw std::runtime_error("Missing ';' after .php directive");
            php = tokens[index + 1];
            std::cout << "php ==> " << php << std::endl;
            index += 2;
        }
        else if (tokens[index] == ".py")
        {
            if (tokens[index + 2] != ";")
                throw std::runtime_error("Missing ';' after .py directive");
            py = tokens[index + 1];
            index += 2;
        }
        else if (tokens[index] == ".pl")
        {
            if (tokens[index + 2] != ";")
                throw std::runtime_error("Missing ';' after .pl directive");
            pl = tokens[index + 1];
            index += 2;
        }
        else if (tokens[index] == "}")
            return true;
        else
            throw std::runtime_error("Error for parsing5");
        index++;
    }
    return false;
}