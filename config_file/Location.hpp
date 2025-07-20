#pragma once

// #include "Server.hpp"
#include <iostream>
#include <vector>
#include <map>
#include "Cgi.hpp"

class Location {
public:
    std::string path;                     // The location path (e.g. "/admin")
    std::string root;                     // The root for this location
    std::vector<std::string> index_files; // Index files: index.html, etc.
    bool autoindex;                      // autoindex on/off
    std::map<int, std::string> return_value;          // return directive (like redirection)
    std::string upload_store;
    std::vector<std::string> allowed_methods; // GET, POST, DELETE...
    cgi_class cgi;
    bool cgi_flag;

    Location(); // Constructor
    bool parser_location(size_t &index, std::vector<std::string> &tokens);
    void print_location();

};