#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include "HTTP/StreamHTTPRequestParser.h"
#include "HttpProxy.h"
#include <sys/signalfd.h>
#include <csignal>
#include <netdb.h>
#include "URI/URI.h"
#include <error.h>
#include <sys/types.h>
#include <chrono>
#include <utility>
#include <sstream>
#include <iomanip>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/signal.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>

HttpProxy::cacheEntry::cacheEntry(HTTPResponse* r, std::string p, long e) {
    this->res = r;
    this->path = std::move(p);
    this->expireAt = e;
}

HttpProxy::cacheEntry::~cacheEntry() {
    delete this->res;
}

HttpProxy::HttpProxy(unsigned int bind_address, int port, int verbose_level) {
    this->verbosity = verbose_level;
    this->prepareMainSocket(bind_address, port);
    this->prepareSignalHandling();
    this->prepareProxyResponses();
}

void HttpProxy::run() {
    while (this->loop) {
        int poller = poll(this->sockets, MAX_CONNECTIONS, 10000);
        if (poller < 0) {
            perror("Cannot poll");
        }
        for (pollfd &event: this->sockets) {
            if (event.fd == this->main_socket && event.revents & POLLIN) {
                this->handleIncomingConnection();
            }

            if (event.fd != this->main_socket && event.fd != this->sfd) {
                if (event.revents & POLLIN) {
                    this->handleIncomingData(event); // READ FROM CLIENT CONNECTIONS

                } else if (event.revents & POLLOUT) {
                    this->handleOutgoingData(event); // SEND TO CLIENT
                }
            }

            if (event.fd == this->sfd && event.revents & POLLIN) {
                this->handleSignal(event); // SIGNAL INTERRUPT
                break;
            }
        }
    }
}

int HttpProxy::getEmptyPollSlot() {
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (this->sockets[i].fd == -1) {
            return i;
        }
    }
    return -1;
}

void HttpProxy::registerNewConnection(int pollSlot) {
    struct sockaddr_in client;
    socklen_t size = sizeof(client);
    int con = accept(this->main_socket, (struct sockaddr *) &client, &size);
    fcntl(con, F_SETFL, O_NONBLOCK);

    this->sockets[pollSlot].fd = con;
    this->sockets[pollSlot].events = POLLIN;
    this->connections[con] = new Connection(
            Connection::SERVER,
            con,
            client,
            &this->sockets[pollSlot]
    );
    char addr[255] = {0};
    inet_ntop(AF_INET, &client.sin_addr, addr, sizeof(addr));
    if(this->verbosity) {
        printf("Accepted new connection from %s:%d at poll slot %d, socket %d!\n", addr, ntohs(client.sin_port), pollSlot, con);
    }
}

HTTPResponse* HttpProxy::queryCache(HTTPRequest *req) {
    for (auto it = this->cache.begin(); it != this->cache.end();) {
        if (it->expireAt < time(nullptr)) {
            this->cache.erase(it);
        } else {
            it++;
        }
    }
    for (cacheEntry& e : this->cache) {
        if (req->path == e.path) {
            return e.res;
        }
    }
    return nullptr;
}

void HttpProxy::closeConnection(int socket) {
    if (this->verbosity) {
        printf("Closing connection at socket %d\n", socket);
    }

//    auto socket_listeners = this->getListeners(this->connections[socket]);

    delete this->connections[socket];
    this->connections.erase(socket);
    for (pollfd& cs : this->sockets) {
        if (cs.fd == socket) {
            cs.fd = -1;
            cs.events = 0;
            break;
        }
    }
//    for (auto listener : socket_listeners) {
//        this->closeConnection(listener->socket);
//    }
}

void HttpProxy::stop() {
    this->loop = false;
    printf("Shutting down...\n");
}

void HttpProxy::prepareSignalHandling() {
    // setup signal handling
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGTERM);
    sigprocmask(SIG_SETMASK, &sigset, NULL);

    this->sfd = signalfd(-1, &sigset, 0);

    this->sockets[0].fd = this->sfd;
    this->sockets[0].events = POLLIN;
}

void HttpProxy::prepareMainSocket(unsigned int bind_address, int port) {
    int one = 1;
    this->main_socket = socket(AF_INET, SOCK_STREAM, 0);
    setsockopt(
        this->main_socket,
        SOL_SOCKET,
        SO_REUSEADDR,
        &one,
        sizeof(one)
    );

    this->listen_address.sin_family = AF_INET;
    this->listen_address.sin_port = htons(port);
    this->listen_address.sin_addr = {bind_address};

    int bind_flag = bind(
        this->main_socket,
        (struct sockaddr*)&this->listen_address,
        sizeof(this->listen_address)
    );

    if (bind_flag < 0) {
        perror("Cannot bind to the socket");
        exit(1);
    }

    int listen_flag = listen(this->main_socket, 16);

    if (listen_flag < 0) {
        perror("Cannot listen to the socket");
    }
    for (auto & socket : this->sockets) {
        socket.fd = -1;
        socket.events = 0;
    }

    this->sockets[1].fd = this->main_socket;
    this->sockets[1].events = POLLIN;
}

void HttpProxy::handleIncomingConnection() {
    int emptySlot = this->getEmptyPollSlot();
    if (emptySlot >= 0) {
        this->registerNewConnection(emptySlot);
    }
}

void HttpProxy::handleIncomingData(pollfd event) {
    char buf[BUF_SIZE] = {0};
    ssize_t bytes_read = recv(event.fd, buf, BUF_SIZE, MSG_DONTWAIT);
    Connection *con = this->connections[event.fd];
    if (bytes_read > 0) {
        con->parser->read(std::string(buf, bytes_read));
        // handle next addr request
        if (con->getType() == Connection::SERVER) {

            if (!con->parser->entitiesToHandle.empty() && con->getStatus() == Connection::READY_TO_RECV) {
                if (con->req != nullptr) {
                    delete con->req; // delete old request
                }
                con->req = (HTTPRequest *)con->parser->entitiesToHandle[0];
                con->parser->entitiesToHandle.erase(con->parser->entitiesToHandle.begin()); // TODO: use something faster

                if(this->verbosity) {
                    printf("%s %s\n", con->req->method.c_str(), con->req->path.c_str());
                }

                auto res = queryCache(con->req);
                if (res == nullptr) {
                    this->makeHTTPRequest(con);
                    con->setStatus(Connection::WAITING);
                } else {
                    con->res = res;
                    con->setStatus(Connection::READY_TO_SEND);
                }
            }
        } else if (con->getType() == Connection::CLIENT) { // GOT DATA FROM HOST
            if (!con->parser->entitiesToHandle.empty()) {
                if (con->res != nullptr && !con->res->inCache) {
                    delete con->res;
                }
                con->res = (HTTPResponse*)con->parser->entitiesToHandle[0];
                this->cacheIfCan(con->res, con->req);
                con->parser->entitiesToHandle.erase(con->parser->entitiesToHandle.begin()); // TODO: use something faster
                auto l = this->getListeners(con);
                for (auto clientConnection : l) {
                    clientConnection->res = con->res;
                    clientConnection->observable = con;
                    clientConnection->setStatus(Connection::READY_TO_SEND);
                }
                con->res->inCache = true;
                this->closeConnection(con->socket);
            }
        }
    } else {
        this->closeConnection(event.fd);
    }
}

void HttpProxy::handleOutgoingData(pollfd event) {
    if (this->connections.count(event.fd)) {
        Connection *clientConnection = this->connections[event.fd];

        int32_t last_error;
        socklen_t optsize = 4;
        getsockopt(event.fd, SOL_SOCKET, SO_ERROR, &last_error, &optsize);

        if (last_error != 0) {
            // WE GOT ERROR
            auto connection_listeners = this->getListeners(clientConnection);
            // https://programmerah.com/linux-getsockopt-so_error-values-errno-h-10319/
            int http_code = last_error == ETIMEDOUT ? 504 : 502;
            for (auto l: connection_listeners) {
                l->res = this->proxy_responses[http_code];
                l->setStatus(Connection::READY_TO_SEND);
                this->removeListener(clientConnection, l);
            }

            this->closeConnection(event.fd);
            return;
        }

        if (clientConnection->getStatus() == Connection::READY_TO_SEND) {
            if (clientConnection->getType() == Connection::CLIENT) {
                std::string req = clientConnection->req->toString();
                ssize_t bytes_sent = send(clientConnection->socket, req.c_str(), req.size(), MSG_DONTWAIT);
                if (bytes_sent >= (long) req.size()) {
                    clientConnection->setStatus(Connection::READY_TO_RECV);
                }
            } else if (clientConnection->getType() == Connection::SERVER) {
                std::string res = clientConnection->res->toString();

                ssize_t bytes_sent = send(clientConnection->socket, res.c_str(), res.size(), MSG_DONTWAIT);
                if (bytes_sent >= (long) res.size()) {
                    if (this->listeners.count(clientConnection->observable)) {
                        if (this->listeners[clientConnection->observable].size() == 1) {
                            this->removeListener(clientConnection->observable, clientConnection);
                            if (!clientConnection->res->inCache) {
                                delete clientConnection->res;
                            }
                            clientConnection->res = nullptr;
                        } else {
                            this->removeListener(clientConnection->observable, clientConnection);
                        }
                    }
                    clientConnection->setStatus(Connection::READY_TO_RECV);
                    this->closeConnection(clientConnection->socket);
                }
            }
        }
    }
}

void HttpProxy::handleSignal(pollfd event) {
    struct signalfd_siginfo siginfo;
    if (read(event.fd, &siginfo, sizeof(siginfo)) != sizeof(siginfo)) {
        printf("Error reading signal!\n");
        return;
    }
    this->stop();
}

HttpProxy::~HttpProxy() {
    close(this->main_socket);
    close(this->sfd);
    for (auto const& [key, connection] : this->connections) {
        shutdown(connection->socket, SHUT_RDWR);
        close(connection->socket);
    }
    this->cleanupProxyResponses();
}

void HttpProxy::makeHTTPRequest(Connection *clientConnection) {

    // GET IP ADDRESS OF HOST
    URI uri = URI::Parse(clientConnection->req->path);
    addrinfo hints{};
    hints.ai_family = AF_INET;
    hints.ai_protocol = IPPROTO_TCP;
    hints.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG;
    addrinfo *resolved;

    int res = getaddrinfo(uri.Host.c_str(), uri.Port.c_str(), &hints, &resolved);
    if(res) {
        error(1,0,"Getaddrinfo failed: %s\n", gai_strerror(res));
    }
    if(!resolved) {
        error(1,0,"Empty result\n");
    }
    auto* addr = (sockaddr_in*) resolved->ai_addr; // <- rzutowanie bezpieczne,

    // CONNECT TO HOST
    int con = socket(AF_INET, SOCK_STREAM, 0);
    fcntl(con, F_SETFL, O_NONBLOCK);
//    int synRetries = 2;
//    setsockopt(con, IPPROTO_TCP, TCP_SYNCNT, &synRetries, sizeof(synRetries));
//    struct timeval timeout;
//    timeout.tv_sec = 60;
//    timeout.tv_usec = 0;
//    setsockopt(con, SOL_SOCKET, SO_SNDTIMEO, &timeout, sizeof(timeout));
//    setsockopt(con, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));

    // https://stackoverflow.com/questions/4181784/how-to-set-socket-timeout-in-c-when-making-multiple-connections
    int flag = connect(con, (sockaddr*)addr, sizeof(*addr));
    if (flag < 0 && errno != EINPROGRESS) {
        perror("Connect to host");
    }

    int pollSlot = this->getEmptyPollSlot();
    this->sockets[pollSlot].fd = con;
    this->sockets[pollSlot].events = POLLOUT;
    this->connections[con] = new Connection(
            Connection::CLIENT,
            con,
            *addr,
            &this->sockets[pollSlot]
    );
    this->connections[con]->req = clientConnection->req;

    this->connections[con]->setStatus(Connection::READY_TO_SEND);
    this->addListener(this->connections[con], clientConnection);
    freeaddrinfo(resolved);
}

void HttpProxy::addListener(Connection *observable, Connection *observer) {
    this->listeners[observable].push_back(observer);
}

void HttpProxy::removeListener(Connection *observable, Connection *observer) {
    if (this->listeners.count(observable)) {
        for (auto it = this->listeners[observable].begin(); it != this->listeners[observable].end(); it++) {
            if (*it == observer) {
                this->listeners[observable].erase(it);
                return;
            }
        }
    }
}

std::vector<Connection *> HttpProxy::getListeners(Connection *observable) {
    return this->listeners[observable];
}

void HttpProxy::prepareProxyResponses() {
    auto* res = new HTTPResponse();
    res->statusCode = 504;
    res->statusText = "Gateway Timeout";
    res->inCache = true;
    res->version = "HTTP/1.1";
    res->data = "<h1>504 Gateway Timeout</h1>";
    res->addHeader("Content-length", std::to_string(res->data.length()));
    this->proxy_responses[504] = res;

    res = new HTTPResponse();
    res->statusCode = 502;
    res->statusText = "Bad Gateway";
    res->inCache = true;
    res->version = "HTTP/1.1";
    res->data = "<h1>502 Bad Gateway</h1>";
    res->addHeader("Content-length", std::to_string(res->data.length()));
    this->proxy_responses[502] = res;
}

void HttpProxy::cleanupProxyResponses() {
    for(auto const& [http_code, response] : this->proxy_responses) {
        delete response;
    }
}

void HttpProxy::cacheIfCan(HTTPResponse* res, HTTPRequest* req) {
    if (res->getHeader("Cache-Control") != nullptr) {
        std::string header = HTTP::tolower(res->getHeader("Cache-Control")->value);
        std::vector<std::string> directives;
        std::string delimiter = ",";
        size_t pos = 0;
        std::string token;
        while ((pos = header.find(delimiter)) != std::string::npos) {
            token = header.substr(0, pos);
            directives.push_back(token);
            header.erase(0, pos + delimiter.length());
        }
        directives.push_back(header);
        for (auto directive : directives) {
            HTTP::trim(directive);
            if (directive.rfind("smax-age", 0) == 0) {
                    long age;
                    try {
                        age = std::stoi(header.substr(1 + header.find('=')));
                    } catch (const std::invalid_argument& e) {
                        return;
                    }
                    if (age <= 0) {
                        return;
                    }
                    this->addToCache(res, req->path, age);
            } else if (directive.rfind("max-age", 0) == 0) {
                long age;
                try {
                    age = std::stoi(header.substr(1 + header.find('=')));
                } catch (const std::invalid_argument& e) {
                    return;
                }
                if (age <= 0) {
                    return;
                }
                this->addToCache(res, req->path, age);
            } else if (directive == "no-cache" || directive == "no-store" || directive == "private") {
                return;
            }
        }
    } else if (res->getHeader("Expires") != nullptr) {
        auto header =  res->getHeader("Expires")->value;
        long age;
        try {
            age = std::stoi(header.substr(1 + header.find('=')));
        } catch (const std::invalid_argument& e) {
            std::tm tm = {};
            std::stringstream ss(header);
            ss >> std::get_time(&tm, "%a, %d %b %Y %H:%M:%S GMT");
            auto expires = std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::from_time_t(std::mktime(&tm)).time_since_epoch()).count();
            age = expires - time(nullptr);
        }
        if (age > 0) {
            this->addToCache(res, req->path, age);
        }
    }
}

void HttpProxy::addToCache(HTTPResponse *res, std::string path, long age) {
    res->inCache = true;
    long expireAt = age + time(nullptr);
    this->cache.emplace_back(res, std::move(path), expireAt);
}
