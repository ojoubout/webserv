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
        std::ifstream file;
        struct stat fileStat;
        std::string basePath;

        const Config * server;
    public:
        Buffer buffer;
        Response();
        Response(Response const &);
        Response(Request const &, const Config *);
        ~Response();
        Response &operator= (Response const &);
        void handleGetRequest(Request const &);
        void handlePostRequest(Request const &);
        void handleDeleteRequest(Request const &);
        std::string HeadertoString() const;
        void    send_file(Socket & connection);
        void    readFile();
        const std::ifstream & getFile() const;
        std::string getIndexFile(std::string);

        void setServerConfig(Config * config);
};

std::string errorPage(const StatusCodeException & e);
#endif