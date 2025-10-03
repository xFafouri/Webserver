#pragma once

#include <map>
#include <vector>
#include <iostream>
#include <unistd.h>

class cgi_class
{
    public:
        std::string php;
        std::string py;
        std::string pl;

        bool parse_cgi(std::vector<std::string> &tokens, size_t &index);
        void cgi_print();
};
