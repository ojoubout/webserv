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

Response::Response(Request const & req, const Config * config) : status(HttpStatus::StatusCode(200)), server(config), stream(NULL)
{
	const Config * location = getLocation(req, config);
	location = location ?: server;

	try {

		if (req.getHeader("Host") == "") {
			throw StatusCodeException(HttpStatus::BadRequest);
		}
		if (req.getMethod() == GET) {
			this->handleGetRequest(req, location);
		} else if (req.getMethod() == POST) {
			this->handlePostRequest(req, location);
		} else if (req.getMethod() == DELETE) {
			this->handleDeleteRequest(req, location);
		}
	} catch (const StatusCodeException & e) {
		setErrorPage(e, location);
	}
}

// static const void handleRequest(const Request & req, const Config * location) {	
// 	if (req.getMethod() == GET) {
// 		this->handleGetRequest(req, location);
// 	} else if (req.getMethod() == POST) {
// 		this->handlePostRequest(req, location);
// 	} else if (req.getMethod() == DELETE) {
// 		this->handleDeleteRequest(req, location);
// 	}
// }

const std::string Response::getRequestedPath(const Request & req, const Config * location) {
	const std::string path = getPathFromUri(req.getRequestTarget());
	struct stat buffer;

	std::string requested_path = location->root;
	requested_path += location->uri != "" ? path.substr(location->uri.length()) : path;
	// dout << "SUBSTR: " << location->uri << " " << location->uri.length() << " " << path.substr(location->uri.length()) << std::endl;

	dout << "Requested File: " << requested_path << std::endl;
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
		dout << "in for loop :" << (filename + index[i])  << std::endl;
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
	std::string filename = getRequestedPath(req, location);

	std::ostringstream oss("");

	dout << "root: " << filename << std::endl;
	// this->basePath = Utils::getFilePath(req.getRequestTarget().substr(1));
	// dout << "*" << basePath << "*" << std::endl;
	std::fstream * file = new std::fstream();
	file->open(filename.c_str());
	if (file->is_open()) {
		stream = file;
	} else {
		if (errno == ENOENT) {
			throw StatusCodeException(HttpStatus::NotFound);
		} else if (errno == EPERM) {
			throw StatusCodeException(HttpStatus::Forbidden);
		} else if (errno == ENAMETOOLONG) {
			throw StatusCodeException(HttpStatus::URITooLong);
		}
	}
	stat (filename.c_str(), &this->fileStat);
	dout << "filename: " << filename << std::endl;
	Utils::fileStat(filename, fileStat);
	dout << "route : "<< Utils::getRoute(req.getHeader("Referer")) << std::endl;
	if (S_ISDIR(fileStat.st_mode) && filename[filename.length() - 1] != '/' && filename != ".")
	{
		throw StatusCodeException(HttpStatus::MovedPermanently, '/' + filename + '/'); 
	}
	if (S_ISDIR(fileStat.st_mode))
	{
		filename = getIndexFile(filename);
		// dout <<"filename: *" <<  filename <<"*" << std::endl;
		file->close();
		file->open(filename.c_str());
		stat (filename.c_str(), &this->fileStat);
		// oss.str("");
		// oss << this->fileStat.st_size;
	}
	oss << this->fileStat.st_size;
	// dout << "SIZE: " <<  this->fileStat.st_size << std::endl;
	// dout << "FILE: " <<  filename.c_str() << std::endl;
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

const std::iostream * Response::getFile() const {
	return stream;
}

void    Response::send_file(Socket & connection)
{
	const int SIZE = 2;
	char buff[SIZE];	

	ssize_t sent = 0;
	ssize_t temp = 0;
	stream->read(buff, SIZE);
	std::streamsize s = ((*stream) ? SIZE : stream->gcount());

	int ret = ::send(connection.getFD(), buff, s, 0);

	if (ret != -1) {
		std::streampos lost = s - ret;
		stream->seekg(stream->tellg() - lost);
	}

	// dout << ret << "\n";
}

void    Response::readFile() {

	buffer.resize(1024);
	stream->read(buffer.data, buffer.size);
	std::streamsize s = ((*stream) ? buffer.size : stream->gcount());
	buffer.resize(s);
}

std::stringstream * errorTemplate(const StatusCodeException & e) {
	std::stringstream * alloc = new std::stringstream("");
	std::stringstream & body = *alloc;
	
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

	return &body;
}

void Response::setErrorPage(const StatusCodeException & e, const Config * location) {
		status = e.getStatusCode();

		_headers["Connection"] = "keep-alive";
		_headers["Content-Type"] = "text/html";
		_headers["Date"] = Utils::getDate();
		_headers["Server"] = SERVER_NAME;

		if (e.getLocation() != ""){
			_headers["Location"] = e.getLocation();
			dout << "Location: " << e.getLocation() <<  CRLF;
		}

		std::stringstream * err;

		const std::map<int, std::string> & error_page = location->error_page.empty() ? server->error_page : location->error_page;
		std::fstream * errPage = NULL;

		if (error_page.find(status) != error_page.end()) {
			errPage = new std::fstream();
			std::string filename = location->root;

			filename += error_page.find(status)->second;
			errPage->open(filename.c_str());
		}

		if (!errPage || !errPage->is_open()) {
			stream = errorTemplate(e);
			if (errPage) {
				delete errPage;
			}
		} else {
			stream = errPage;
		}
}
