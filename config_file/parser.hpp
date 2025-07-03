#pragma once 

#include <algorithm>
#include <ctime>
#include <stdexcept>
#include <utility>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>
#include "Server.hpp"

class Parser
{
    protected:
        std::vector<std::string> tokens;
    public:
        std::vector<Server> servers;
        void    parsing(char *fileName);
        void    parse();
};

