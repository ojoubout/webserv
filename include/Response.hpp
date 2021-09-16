#ifndef RESPONSE_HPP
# define RESPONSE_HPP

# include <fstream>
# include <string>
# include <sys/types.h>
# include <sys/stat.h>
# include <unistd.h>
# include <sstream>

# include "Buffer.hpp"
# include "Request.hpp"
# include "Socket.hpp"
# include "Utils.hpp"
# include "MimeTypes.hpp"
// #include

class Response : public Message
{
    private:
        HttpStatus::StatusCode status;
        std::iostream * stream;
        struct stat fileStat;
        std::string basePath;

        const Config * server;

        const std::string getRequestedPath(const Request &, const Config *);
    public:
        Buffer buffer;
        Response();
        Response(Response const &);
        Response(Request const &, const Config *);
        ~Response();
        Response &operator= (Response const &);
        void handleGetRequest(Request const &, const Config *);
        void handlePostRequest(Request const &, const Config *);
        void handleDeleteRequest(Request const &, const Config *);
        std::string HeadertoString() const;
        void    send_file(Socket & connection);
        void    readFile();
        const std::iostream * getFile() const;
        std::string getIndexFile(std::string);
        void setErrorPage(const StatusCodeException & e, const Config * location);

        void setServerConfig(const Config * config);
        const Config * getServerConfig() const;
};

std::stringstream * errorPage(const StatusCodeException & e);
static const Config * getLocation(const Request & req, const Config * server);
static const std::string getPathFromUri(const std::string & uri);
static const void handleRequest(const Request & req, const Config * location);

#endif