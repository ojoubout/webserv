#include "Response.hpp"
#include "debug.hpp"

Response::Response() : _is_cgi(false), status(HttpStatus::StatusCode(200))
{
}

Response::Response(Response const & src) : _is_cgi(false)
{
	*this = src;
}

Response::~Response(){}

Response &Response::operator=(Response const & src)
{
	buffer_header = src.buffer_header;
	buffer_body = src.buffer_body;
	status = src.status;
	_server = src._server;
	_is_cgi = src._is_cgi;
	return (*this);
}

void Response::reset() {
    Message::reset();
	_is_cgi = false;
	status = HttpStatus::StatusCode(200);
	buffer_header.resize(0);
	buffer_body.resize(0);
}
// Response::Response(Request const & req, const Config * config) 
// {
// }

void Response::handleRequest(Request const & req) {
	this->_server = req.getServerConfig();
	this->_location = req.getLocation();
	// const Config * location = getLocation(req, _server);

	// const std::string request_path = getRequestedPath(req, location);

	std::cout <<  "****" << req.getRequestTarget() << std::endl;
	std::cout << _location->root << std::endl;
	std::cout << req.getFilename() << std::endl;

	if (req.getMethod() == GET)
		this->handleGetRequest(req);
	if (req.getMethod() == POST)
		this->handlePostRequest(req);
	if (req.getMethod() == DELETE)
		this->handleDeleteRequest(req);
}

void Response::handleGetRequest(Request const & req)
{
	std::string filename = req.getFilename();
	std::ostringstream oss("");
	std::map<std::string, Config>::const_iterator cgi_location;
	struct stat fileStat;

	std::fstream * file = new std::fstream();
	file->open(filename);
	delete _body;
	_body = file;
	stat (filename.c_str(), &fileStat);

	for (std::map<std::string, Config>::const_iterator it = _server->location.begin(); it != _server->location.end(); ++it) {
		if (it->first == Utils::getFileExtension(filename)) {
			_location = &it->second;
			break;
		}
	}
	// std::cout << "lol: " << location-> << std::endl;
	// if (Utils::getFileExtension(filename) == ".php")
	if (_server->location.find(Utils::getFileExtension(filename)) != _server->location.end())
	{
		char buff[101] = {0};
		_is_cgi = true;
		file->close();

		// std::map<std::string, std::string> env;
		gethostname(buff, 100);
		// env.insert(std::make_pair("REQUEST_METHOD", "GET"));
		// env.insert(std::make_pair("PATH", getenv("PATH")));
		// env.insert(std::make_pair("TERM", getenv("TERM")));
		// env.insert(std::make_pair("HOME", getenv("HOME")));
		// env.insert(std::make_pair("HOSTNAME", hostname));
		// env.insert(std::make_pair("QUERY_STRING", req.getRequestTarget().substr(req.getRequestTarget().find_first_of('?'))));

		
		char * const ar[4] = {const_cast<char *>(_location->cgi.c_str()), const_cast<char *>(filename.c_str()), NULL};
		pipe(fd);
		pid = fork();
		if (pid == 0)
		{
			std::vector<const char *> v;
			v.push_back(strdup((std::string("REQUEST_METHOD") + "=" + "GET").c_str()));
			v.push_back(strdup((std::string("PATH") + "=" + getenv("PATH")).c_str()));
			v.push_back(strdup((std::string("TERM") + "=" + getenv("TERM")).c_str()));
			v.push_back(strdup((std::string("HOME") + "=" + getenv("HOME")).c_str()));
			gethostname(buff, 100);
			v.push_back(strdup((std::string("HOSTNAME") + "=" + buff).c_str()));
			getlogin_r(buff, 100);
			v.push_back(strdup((std::string("USER") + "=" + buff).c_str()));
			v.push_back(strdup((std::string("SCRIPT_FILENAME") + "=" + filename).c_str()));
			size_t n = req.getRequestTarget().find_first_of('?') + 1;
			n = n == std::string::npos ? req.getRequestTarget().length() : n;
			v.push_back(strdup((std::string("QUERY_STRING") + "=" + req.getRequestTarget().substr(n)).c_str()));
			v.push_back(NULL);		
			close(fd[0]);
			dup2(fd[1], 1);
			execve(ar[0], ar, const_cast<char * const *>(v.data()));
			close(fd[1]);
			return ;
		}
	}
	oss << fileStat.st_size;
	// insert_header("Content-Length", oss.str());
	insert_header("Date", Utils::getDate());
	insert_header("Server", SERVER_NAME);
	insert_header("Last-Modified", Utils::time_last_modification(fileStat));
	insert_header("Transfer-Encoding", "chunked");
	const char * type = MimeTypes::getType(filename.c_str());
	// type = type ?: "text/plain";
	// if (_is_cgi)
		// type = "text/html; charset=UTF-8";
	// insert_header("Content-Type", type);
	insert_header("Connection", "keep-alive");
	insert_header("Accept-Ranges", "bytes");
}

void Response::handlePostRequest(Request const & req)
{

}

void Response::handleDeleteRequest(Request const & req)
{

}

std::string Response::HeadertoString()
{
	std::ostringstream response("");
	// Buffer buff;

	response << "HTTP/1.1 " << this->status << " " << reasonPhrase(this->status) << CRLF;
	for (std::map<std::string, std::string>::const_iterator it = _headers.begin(); it != _headers.end(); ++it)
	{
		response << it->first << ": " << it->second << CRLF;
	}

	if (is_cgi())
	{
		char s[2050] = {0};
		// buff.resize(1025);
		// buff.data[1024] = 0;
		int ret = 0, total = 0;
		size_t pos;
		while (true)
		{
			ret = read(fd[0], s + total, 2049 - total);
			total += ret;
			s[total] = 0;
			if ((pos = std::string(s).find("\r\n\r\n")) != std::string::npos || (pos = std::string(s).find("\n\n")) != std::string::npos )
				break ;
		}
		// size_t pos = std::string(s).find("\r\n\r\n");
		std::string token = std::string(s).substr(0, pos);
		buffer_body.setData(std::string(s).substr(pos).c_str(), std::string(s).substr(pos).length());
		response << token << CRLF;

		// strcpy(buffer_body.data, std::string(buff.data).substr(0, pos).c_str());
	}
	// if (is_cgi())
	// {
	// 	while (true)
	// 	{
	// 		int ret = read(fd[0], &c, 1);
	// 		std::cout << ret << std::endl;
	// 		if ((c == '\n' && header_line.length() == 0) || ret <= 0)
	// 			break ;
	// 		if (c == '\n')
	// 		{
	// 			response << header_line << CRLF;
	// 			header_line.clear();
	// 		}
	// 		header_line += c;
	// 	}
	// }
	// if (!is_cgi())
		response << CRLF;
	return (response.str());
}

const std::iostream * Response::getFile() const {
	return _body;
}

// void    Response::send_file(Socket & connection)
// {
// 	const int SIZE = 2;
// 	char buff[SIZE];	

// 	ssize_t sent = 0;
// 	ssize_t temp = 0;
// 	stream->read(buff, SIZE);
// 	std::streamsize s = ((*stream) ? SIZE : stream->gcount());

// 	int ret = ::send(connection.getFD(), buff, s, 0);

// 	if (ret != -1) {
// 		std::streampos lost = s - ret;
// 		stream->seekg(stream->tellg() - lost);
// 	}

// 	// dout << ret << "\n";
// }

void	Response::readFile() {

	if (!_is_cgi)
	{
		buffer_body.resize(1024);
		_body->read(buffer_body.data, buffer_body.size);
		std::streamsize s = ((*_body) ? buffer_body.size : _body->gcount());
		buffer_body.resize(s);
	}
	else
	{
		close(fd[1]);
		// waitpid(pid, NULL, 0);
		buffer_body.resize(1024);
		int ret = read(fd[0], buffer_body.data, 1024);
		if (ret == 0) {
			_is_cgi = false;
		}
		if (ret != 1024) {
			buffer_body.resize(ret);
		}
		// buffer << std::hex << ret;
		// buffer_body.data[ret] = 0;
		// printf("%d -> %s", ret,buffer.data);
	}
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

	const std::map<int, std::string> & error_page = location->error_page.empty() ? _server->error_page : location->error_page;
	std::fstream * errPage = NULL;

	if (error_page.find(status) != error_page.end()) {
		errPage = new std::fstream();
		std::string filename = location->root;

		filename += error_page.find(status)->second;
		errPage->open(filename.c_str());
	}

	delete _body;
	if (!errPage || !errPage->is_open()) {
		_body = errorTemplate(e);
		if (errPage) {
			delete errPage;
	}
	} else {
		_body = errPage;
	}

	// _body->seekg(0, _body->beg);
	// std::cout << "Content-Length: " << _body->tellp() << std::endl;
	// _headers["Content-Length"] = Utils::to_str(_body->tellp());
	_headers["Transfer-Encoding"] = "chunked";

}

char* formatdate(char* str, time_t val)
{
        strftime(str, 250, "%d-%b-%Y %H:%M", localtime(&val));
        return str;
}

char *getFileCreationTime(const char *path, char *format) 
{
    struct stat attr;
    stat(path, &attr);
	return formatdate(format, attr.st_mtime);
}


std::string listingPage(const ListingException & e)
{
	std::ostringstream header("");
	std::ostringstream body("");

	header << "HTTP/1.1 " << 200 << " " << "OK" << CRLF;
	header << "Connection: keep-alive" << CRLF;
	header << "Content-Type: text/html" << CRLF;
	header << "Date: " << Utils::getDate() <<  CRLF;
	header << "Server: " << SERVER_NAME << CRLF ;

	DIR *dir;
	struct dirent *ent;
	body << "<!DOCTYPE html>\n" ;
	body << "<html>\n";
	body << "<head><title>Index of " << e.getReqTarget() << "</title></head>\n";
	body << "<body bgcolor=\"white\">\n";
	body << "<h1>Index of " << e.getReqTarget() << "</h1><hr><pre><a href=\"" << e.getReqTarget().substr(0, e.getReqTarget().find_last_of('/', e.getReqTarget().length() - 2)) << "/\">../</a>\n";

	if ((dir = opendir (e.what())) != NULL) {
		while ((ent = readdir (dir)) != NULL) {
			struct stat attr;
			char format[250];
			std::string file_path = e.getPath() + std::string(ent->d_name);
			if (!strcmp(".", ent->d_name) || !strcmp("..", ent->d_name) || !(stat(file_path.c_str(), &attr) == 0 && S_ISDIR(attr.st_mode)))
				continue ;
			body << "<a href=\"" << ent->d_name << "/\">" << ent->d_name << "/</a> " << std::setw(69 - strlen(ent->d_name)) << getFileCreationTime(file_path.c_str(), format);
			body << "                   -\n";
		}
		closedir (dir);
	}

	if ((dir = opendir (e.what())) != NULL) {
		while ((ent = readdir (dir)) != NULL) {
			char format[250];
			struct stat attr;
			std::string file_path = e.getPath() + std::string(ent->d_name);
			if (!strcmp(".", ent->d_name) || !strcmp("..", ent->d_name) || (stat(file_path.c_str(), &attr) == 0 && S_ISDIR(attr.st_mode)))
				continue ;
			body << "<a href=\"" << ent->d_name << "\">" << ent->d_name << "</a> " << std::setw(70 - strlen(ent->d_name)) << getFileCreationTime(file_path.c_str(), format);
			body << std::setw(20);
			body << attr.st_size;
			body << "\n";
		}
		closedir (dir);
	}
	body << "</pre><hr></body>\n</html>\n";
	header << "Content-Length: " << body.str().length() << CRLF << CRLF;
	header << body.str();
	return header.str();
}

bool Response::is_cgi() const
{
	return this->_is_cgi;
}
