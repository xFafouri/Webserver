#pragma once

// #include "Server.hpp"
#include <iostream>
#include <vector>
#include <map>
#include "Cgi.hpp"

class Location {
public:
    std::string path;                     
    std::string root;               
    std::vector<std::string> index_files; 
    bool autoindex;                  
    bool upload_flag;
    std::map<int, std::string> return_value;  
    std::string upload_store;
    std::vector<std::string> allowed_methods; 
    cgi_class cgi;
    bool cgi_flag;

    Location();
    bool parser_location(size_t &index, std::vector<std::string> &tokens);
    void print_location();

};