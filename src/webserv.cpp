#include "webserv.hpp"

// std::vector<Socket> sockets;
socklen_t addrlen = sizeof(sockaddr_in);

void error(std::string message) {
	std::cerr << "webserv: " << message << ". " << std::strerror(errno) << ". " << errno << std::endl;
	for (std::vector<Config>::const_iterator it = servers.begin(); it != servers.end(); ++it) {
		it->socket->close();
	}
	return exit(EXIT_FAILURE);
}

static std::string getResponse(std::string request) {
	
	std::ostringstream response("");
	std::string response_body = "<html>\n"
					"<body>\n"
					"<h1>Hello, World!</h1>\n"
					"</body>\n"
					"</html>";

	
	response	<< "HTTP/1.1 200 OK\n"
			<< "Date: " << Utils::getDate() << "\n"
			<< "Server: WebServer (MacOs)\n"
			// << "Content-Length: " << response_body.length() << "\n"
			<< "Last-Modified: Wed, 22 Jul 2009 19:15:56 GMT\n"
			<< "Transfer-Encoding: chunked\n"
			<< "Content-Type: text/html\n"
			<< "Connection: Closed\n\n"
			<< Utils::chuncked_transfer_encoding(response_body);
	return response.str();
}

void setup_server(Config & conf) {
	int opt = 1;

	conf.socket = new Socket();
	conf.socket->create(AF_INET, SOCK_STREAM, 0);
	conf.socket->setState(NonBlockingSocket);
	conf.socket->setOption(SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	conf.socket->setAddress(AF_INET, inet_addr(conf.host.c_str()), conf.port);
	conf.socket->bind();
	conf.socket->listen(10);
}

void handle_signal(int sig) {
	std::cout << "SIGNAL " << sig << std::endl;
}

void exit_program(int sig) {
	for (std::vector<Config>::const_iterator it = servers.begin(); it != servers.end(); ++it) {
		it->socket->close();
		std::cerr << "Close Socket: " << it->socket->getFD() << std::endl;
	}
	return exit(EXIT_SUCCESS);
}

Socket * isSocket(int fd) {
	for (std::vector<Config>::const_iterator it = servers.begin(); it != servers.end(); ++it) {
		if (it->socket->getFD() == fd) {
			return it->socket;
		}
	}
	return NULL;
}


Socket * socketExists(const std::vector<Config>::iterator & curr_it) {
	for (std::vector<Config>::const_iterator it = servers.begin(); it != curr_it; ++it) {
		if (it->socket->getHost() == curr_it->host && it->socket->getPort() == curr_it->port) {
			return it->socket;
		}
	}
	return NULL;
}

const Config * getConnectionServerConfig(const Socket * socket, const Request * request) {
	const Config * default_server = NULL;

	for (std::vector<Config>::const_iterator it = servers.begin(); it != servers.end(); ++it) {
		if (it->host == socket->getHost() && it->port == socket->getPort()) {
			if (default_server == NULL) {
				default_server = &(*it);
				if (request == NULL) break;
			}
			if (it->server_name == request->getHeader("Host")) {
				return &(*it);
			}
		}
	}
	return default_server;
}

int main(int argc, char *argv[]) {
	std::vector<pollfd> fds;
	std::map<int, Response*> responses;
	std::map<int, Request*> requests;
	std::map<int, Socket> connections;

	open_config_file(argc, argv);

	// std::cerr << servers[0].host << " " << htonl(inet_addr(servers[0].host.c_str())) << ":" << servers[0].port << std::endl;

	for (std::vector<Config>::iterator it = servers.begin(); it != servers.end(); ++it) {
		Socket * sock = socketExists(it);
		if (!sock) { // checks if host:port already exists 
			setup_server(*it);
			fds.push_back((pollfd){it->socket->getFD(), POLLIN});
			std::cerr << "Listening on " << it->host << ":" << it->port << std::endl;
		} else { // else copy the already existed socket 
			it->socket = sock;
		}
	}

	signal(13, handle_signal);
	signal(1, exit_program);
	signal(2, exit_program);

	while (true) {

		// examines to all socket & connections if they are ready
		int pret = poll(&fds.front(), fds.size(), -1);
	
		if (pret == -1) {
			error("poll() failed");
		}

		// loop through all sockets & connections
		int curr_size = fds.size();
		for (int i = 0; i < curr_size; ++i) {

			if (fds[i].revents == 0) {
				continue;
			}
			// if (fds[i].revents != 4)
			// 	std::cerr << "CONNECTION: " << i << " " << fds[i].revents << std::endl;
			// new connection
			Socket * sock;
			if ((sock = isSocket(fds[i].fd))) {
				Socket new_connection;
				do {	
					new_connection = sock->accept();
					if (new_connection.getFD() != -1) {
						std::cerr << "Socket: " << sock->getHost() << ":" << sock->getPort() << ", ";
						std::cerr << "New Connection: " << new_connection.getHost() << ":" << new_connection.getPort() << std::endl;
						new_connection.setState(NonBlockingSocket);
						new_connection.setSocket(sock);
						fds.push_back((pollfd){new_connection.getFD(), POLLIN});
						connections.insert(std::make_pair(new_connection.getFD(), new_connection));
					}

				} while (new_connection.getFD() != -1);
			// new data from a connection
			} else {

				Socket & connection = connections[fds[i].fd];
				bool close = false;
				Response * response = NULL;
				Request * request = NULL;
				// connection.setFD(fds[i].fd);
				// connection->setState(NonBlockingSocket);

				if (fds[i].revents & POLLIN) {
					// std::cerr << message << std::endl;
					try {
						if (requests.find(connection.getFD()) != requests.end()) {
							request = requests.find(connection.getFD())->second;
							request->receive(connection);
						} else {
							request = new Request(connection);
							requests.insert(std::make_pair(connection.getFD(), request));
						}

						if (request->isFinished()) {
							std::cerr << "Request Succesful" << std::endl;
							response = new Response(*request, getConnectionServerConfig(connection.getSocket(), request));
						}
					} catch (const StatusCodeException & e) {
						std::cerr << "Caught exception: " << e.getStatusCode() << " " << e.what() << std::endl;
						response = new Response();

						response->setServerConfig(getConnectionServerConfig(connection.getSocket(), NULL));

						response->setErrorPage(e, response->getServerConfig());
					}
					if (response) {
						std::string data = response->HeadertoString();

						response->buffer.setData(data.c_str(), data.length());
						responses.insert(std::make_pair(connection.getFD(), response));
						requests.erase(connection.getFD());
						delete request;
						fds[i].events = POLLOUT;
					}
				}

				if (responses.find(connection.getFD()) != responses.end()) {
					response = responses.find(connection.getFD())->second;
				}

				if (fds[i].revents & POLLOUT) {
					
					
					if (response->buffer.length() == 0 && !response->getFile()->eof()) {
						response->readFile();
					}

					connection.send(response->buffer);

					// if ((!response->getFile() || (response->getFile() && !(*response->getFile()))) && response->buffer.length() == 0) {
					if (response->getFile()->eof() && response->buffer.length() == 0) {
						close = true;
					}
				}
				if (fds[i].revents & POLLHUP) {
					close = true;
				}
				if (close) {
					connection.close();
					fds.erase(fds.begin() + i);
					--i;
					--curr_size;
					responses.erase(connection.getFD());
					delete response;
				}
			}
		}
	}
	// Bind the socket to a IP / port
	// Mark the socket for listening in
	// Accept a call
	// Close the listening socket
	// While receiving - display message, echo message
	// Close socket
	return 0;
}