#include "Config.hpp"

Config::Config() {
    port = 80;
    host = "0.0.0.0";
    root = getcwd(NULL, 0);
    max_body_size = 1000000; // default 1m
}

Config::Config(const Config & config) {
    
    port = config.port;
    host = config.host;
    server_name = config.server_name;
    error_page = config.error_page;
    max_body_size = config.max_body_size;
    location = config.location;
    methods = config.methods;
    root = config.root;
    redirect = config.redirect;
    listing = config.listing;
    index = config.index;
    cgi = config.cgi;
    upload = config.upload;
}

Config::~Config() {

}

