#pragma once

// #include "parser.hpp"
#include <vector>
#include <iostream>
#include <map>
#include "Location.hpp"


class Server
{
    public:
    int listen;
    std::string host;
    std::vector<std::string> server_names;
    std::map<int, std::string> error_pages;
    size_t client_max_body_size;
    std::vector<Location> locations;

    Server();
    bool parsServer(std::vector<std::string>, size_t &index);
};

