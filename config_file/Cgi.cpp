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
        if (tokens[index] == ".php")
        {
            if (index + 2 >= tokens.size())
                throw std::runtime_error("Unexpected end of tokens near 'cgi extension' directive");
            if (tokens[index + 2] != ";")
                throw std::runtime_error("Missing ';' after .php directive");
            std::string path = tokens[index + 1];
            php = tokens[index + 1];
            index += 2;
        }
        else if (tokens[index] == ".py")
        {
            if (index + 2 >= tokens.size())
                throw std::runtime_error("Unexpected end of tokens near 'cgi extension' directive");
            if (tokens[index + 2] != ";")
                throw std::runtime_error("Missing ';' after .py directive");
            std::string path = tokens[index + 1];
            py = tokens[index + 1];
            index += 2;
        }
        else if (tokens[index] == ".pl")
        {
            if (index + 2 >= tokens.size())
                throw std::runtime_error("Unexpected end of tokens near 'cgi extension' directive");
            if (tokens[index + 2] != ";")
                throw std::runtime_error("Missing ';' after .pl directive");
            std::string path = tokens[index + 1];
            pl = tokens[index + 1];
            index += 2;
        }
        else if (tokens[index] == "}")
            return true;
        else
            throw std::runtime_error("Error: Unknown directive in cgi block.");
        index++;
    }
    return false;
}