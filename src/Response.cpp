#include "Response.hpp"
#include "debug.hpp"

Response::Response()
{
}

Response::Response(Response const & src)
{
	*this = src;
}

Response::~Response(){}

Response &Response::operator=(Response const & src)
{
	buffer = src.buffer;
	return (*this);
}

Response::Response(Request const & req, const Config * config) : status(HttpStatus::StatusCode(200)), server(config)
{
	const Config * location = getLocation(req, config);
	location = location ?: server;


	const std::string request_path = getRequestedPath(req, location);


	std::cout << req.getRequestTarget() << std::endl;
	std::cout << location->root << std::endl;
	std::cout << request_path << std::endl;

	if (req.getMethod() == GET)
		this->handleGetRequest(req, location);
	if (req.getMethod() == POST)
		this->handlePostRequest(req, location);
	if (req.getMethod() == DELETE)
		this->handleDeleteRequest(req, location);
}

const std::string Response::getRequestedPath(const Request & req, const Config * location) {
	const std::string path = getPathFromUri(req.getRequestTarget());
	struct stat buffer;

	std::string requested_path = location->root;
	requested_path += location->uri != "" ? path.substr(location->uri.length()) : path;
	// dout << "SUBSTR: " << location->uri << " " << location->uri.length() << " " << path.substr(location->uri.length()) << std::endl;

	std::cerr << "Requested File: " << requested_path << std::endl;
	if (requested_path[requested_path.length() - 1] != '/' && stat(requested_path.c_str(), &buffer) == 0 && S_ISDIR(buffer.st_mode)) {
		throw StatusCodeException(HttpStatus::MovedPermanently, path + '/');
	}

	return requested_path;
}

static const std::string getPathFromUri(const std::string & uri) {
	return uri.substr(0, uri.find_first_of('?'));
}
static const Config * getLocation(const Request & req, const Config * server) {
	size_t len = 0;
	const Config * loc = NULL;
	for (std::map<std::string, Config>::const_iterator it = server->location.begin(); it != server->location.end(); ++it) {
		const std::string path = getPathFromUri(req.getRequestTarget());
		if (it->first.length() <= path.length() && it->first.length() > len) {
			if (path.compare(0, it->first.length(), it->first.c_str()) == 0) {
				len = it->first.length();
				loc = &it->second;
			}
		}
	}
	return loc;
}

std::string Response::getIndexFile(std::string filename)
{
	std::string index[2] = {"index.html", "houssam.html"};

	filename = filename == "." ? "": filename;

	for (int i = 0; i < 2; ++i)
	{
		std::cout << "in for loop :" << (filename + index[i])  << std::endl;
		if (access((filename + index[i]).c_str(), F_OK))
			continue ;
		struct stat buffer;
		stat ((filename + index[i]).c_str(), &buffer);
		if (!(buffer.st_mode & S_IROTH))
			throw StatusCodeException(HttpStatus::Forbidden);
		else
			return (filename + index[i]);
	}
	throw StatusCodeException(HttpStatus::Forbidden); 
}

void Response::handleGetRequest(Request const & req, const Config * location)
{
	std::cout << "root: " << location->root << std::endl;
	std::string filename = req.getRequestTarget().substr(1);
	filename = filename == "" ? ".": filename;
	std::ostringstream oss("");

	// this->basePath = Utils::getFilePath(req.getRequestTarget().substr(1));
	// std::cout << "*" << basePath << "*" << std::endl;
	file.open(filename.c_str());
	stat (filename.c_str(), &this->fileStat);
	std::cerr << "filename: " << filename << std::endl;
	Utils::fileStat(filename, fileStat);
	std::cerr << "route : "<< Utils::getRoute(req.getHeader("Referer")) << std::endl;
	if (S_ISDIR(fileStat.st_mode) && filename[filename.length() - 1] != '/' && filename != ".")
	{
		throw StatusCodeException(HttpStatus::MovedPermanently, '/' + filename + '/'); 
	}
	if (S_ISDIR(fileStat.st_mode))
	{
		filename = getIndexFile(filename);
		// std::cerr <<"filename: *" <<  filename <<"*" << std::endl;
		file.close();
		file.open(filename.c_str());
		stat (filename.c_str(), &this->fileStat);
		// oss.str("");
		// oss << this->fileStat.st_size;
	}
	oss << this->fileStat.st_size;
	// std::cerr << "SIZE: " <<  this->fileStat.st_size << std::endl;
	// std::cerr << "FILE: " <<  filename.c_str() << std::endl;
	insert_header("Content-Length", oss.str());
	insert_header("Date", Utils::getDate());
	insert_header("Server", SERVER_NAME);
	insert_header("Last-Modified", Utils::time_last_modification(this->fileStat));
	// insert_header("Transfer-Encoding", "chunked");
	const char * type = MimeTypes::getType(filename.c_str());
	type = type ?: "text/plain";
	insert_header("Content-Type", type);
	insert_header("Connection", "keep-alive");
	insert_header("Accept-Ranges", "bytes");

}

void Response::handlePostRequest(Request const & req, const Config * location)
{

}

void Response::handleDeleteRequest(Request const & req, const Config * location)
{

}

std::string Response::HeadertoString() const
{
	std::ostringstream response("");

	response << "HTTP/1.1 " << this->status << " " << reasonPhrase(this->status) << CRLF;
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it)
	{
		response << it->first << ": " << it->second << CRLF;
	}
	response << CRLF;
	return (response.str());
}

const std::ifstream & Response::getFile() const {
	return file;
}

void    Response::send_file(Socket & connection)
{
	const int SIZE = 2;
	char buff[SIZE];	

	ssize_t sent = 0;
	ssize_t temp = 0;
	file.read(buff, SIZE);
	std::streamsize s = ((file) ? SIZE : file.gcount());

	int ret = ::send(connection.getFD(), buff, s, 0);

	if (ret != -1) {
		std::streampos lost = s - ret;
		file.seekg(file.tellg() - lost);
	}

	// std::cerr << ret << "\n";
}

void    Response::readFile() {

	buffer.resize(1024);
	file.read(buffer.data, buffer.size);
	std::streamsize s = ((file) ? buffer.size : file.gcount());
	buffer.resize(s);
}

std::string errorPage(const StatusCodeException & e) {
	std::ostringstream body("");
	

	body << "HTTP/1.1 " << e.getStatusCode() << " " << reasonPhrase(e.getStatusCode()) << CRLF;
	body << "Connection: keep-alive" << CRLF;
	body << "Content-Type: text/html" << CRLF;
	if (e.getLocation() != ""){
		body << "Location: " << e.getLocation() <<  CRLF;
		std::cerr << "Location: " << e.getLocation() <<  CRLF;
	}
	body << "Date: " << Utils::getDate() <<  CRLF;
	body << "Server: " << SERVER_NAME << CRLF << CRLF;



	body << "<!DOCTYPE html>\n" ;
	body << "<html lang=\"en\">\n";
	body << "<head>\n";
    body << "<title>" << e.getStatusCode() << "</title>\n";
	body << "</head>\n";
	body << "<body>\n";
    body << "<h1 style=\"text-align:center\">" << e.getStatusCode() << " - " << HttpStatus::reasonPhrase(e.getStatusCode()) << "</h1>\n";
	body << "<hr>\n";
	body << "<h4 style=\"text-align:center\">WebServer</h4>\n";
	body << "</body>\n";

	return body.str();
}

// bool send_buffer(Socket & connection) {
// 	connection.send()
// }