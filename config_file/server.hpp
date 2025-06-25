#pragma once 

#include <algorithm>
#include <ctime>
#include <stdexcept>
#include <utility>
#include <vector>
#include <iostream>
#include <sstream>
#include <fstream>

class Parser
{
    protected:
        std::vector<std::string> tokens;
    public:
        void    parsing(char *fileName);
};


class listen : public Parser
{
    
};