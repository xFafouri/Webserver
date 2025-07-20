#include "Location.hpp"
#include <stdexcept>
#include <map>

Location::Location()
{
    autoindex = false;
}

void fill_methods(std::map<std::string, bool> &methods)
{
    methods.insert(std::make_pair("GET", false));
    methods.insert(std::make_pair("POST", false));
    methods.insert(std::make_pair("DELETE", false));
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
}

bool Location::parser_location(size_t &index, std::vector<std::string> tokens)
{
    path = tokens[index];
    index += 2;
    for(;index < tokens.size(); index++)
    {
        // std::cout << tokens[index] << std::endl;
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
                    break;
                }
                else if (
                    tokens[index] == "root"  || tokens[index] == "autoindex" ||
                    tokens[index] == "allowed_methods" || tokens[index] == "path" || 
                    tokens[index] == "index" || tokens[index] == "upload_store")
                {
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
        else if (tokens[index] == "autoindex")
        {
            if (tokens[index + 2] != ";")
                throw std::runtime_error("error for parsing for autoindex");
            if (tokens[index + 1] == "on")
                autoindex = true;
            else if (tokens[index + 1] == "off")
                autoindex = false;
            else
                throw std::runtime_error("Your autoindex must be 'TRUE' or 'FALSE'");
            index += 2;
        }
        else if (tokens[index] == "upload_store")
        {
            if (tokens[index + 2] != ";")
                throw std::runtime_error("error for parsing for autoindex");
            else
                upload_store = tokens[index + 1];
            index += 2;
        }
        else if (tokens[index] == "allowed_methods")
        {
            index++;
            std::map<std::string, bool> methods;
            fill_methods(methods);
            // std::cout << methods["GET"] << std::endl;
            // std::cout << methods["POST"] << std::endl;
            // std::cout << methods["DELETE"] << std::endl;
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
                index++;
            }
            if (tokens[index] != ";")
                throw std::runtime_error("Missing ';' at the end of Methods directive");
        }
        else if (tokens[index] == "}")
        {
            // std::cout << "here\n";
            return true;
        }
        else
            throw std::runtime_error("Error for parsing7");

    }
    return true; 
}