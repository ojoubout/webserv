#pragma once

#include <string>
#include <vector>
#include <map>
#include <unistd.h>
#include "Request.hpp"

class Config {
public:
    Config();
    Config(const Config &);
    ~Config();
    int port;
    std::string host;

    std::string server_name;
    std::map<int, std::string> error_page;
    size_t max_body_size;    

    std::map<std::string, Config> location;

    std::vector<Method> methods;
    std::string root;
    std::pair<HttpStatus::StatusCode, std::string> redirect;
    bool listing;
    std::vector<std::string> index;
    std::string cgi;
    std::string upload;
};
