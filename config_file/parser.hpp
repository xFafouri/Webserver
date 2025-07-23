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
        std::vector<ServerCo> servers;
        void    parsing(std::string fileName);
        void    parse();
};

