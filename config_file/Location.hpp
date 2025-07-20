#pragma once

// #include "Server.hpp"
#include <iostream>
#include <vector>



class Location {
public:
    std::string path;                     // The location path (e.g. "/admin")
    std::string root;                     // The root for this location
    std::vector<std::string> index_files; // Index files: index.html, etc.
    long client_max_body_size;
    bool autoindex;                      // autoindex on/off
    std::string return_value;            // return directive (like redirection)
    std::vector<std::string> allowed_methods; // GET, POST, DELETE...

    Location(); // Constructor
    bool parser_location(size_t &index, std::vector<std::string> tokens);
};