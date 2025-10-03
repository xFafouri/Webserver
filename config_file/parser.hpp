#pragma once 

#include <algorithm>
#include "Server.hpp"
#include <ctime>
#include <stdexcept>
#include <utility>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include <cstdlib>

class ServerCo;

class Parser
{
    protected:
        std::vector<std::string> tokens;
    public:
        template <typename T>
        std::string to_string(T value) 
        {
            std::ostringstream oss;
            oss << value;
            return oss.str();
        }
        std::vector<ServerCo> servers;
        void    parsing(std::string fileName);
        void check_hosting_port(std::vector<ServerCo> servers);

        void    parse();
};

