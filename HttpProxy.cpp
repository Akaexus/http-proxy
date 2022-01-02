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
#include <ctime>
#include <netdb.h>
#include "URI/URI.h"
#include <error.h>


HttpProxy::cacheEntry::cacheEntry(HTTPResponse* r, std::string p, long e) {
    this->res = r;
    this->path = std::move(p);
    this->expireAt = e;
}

HttpProxy::cacheEntry::~cacheEntry() {
    delete this->res;
}

HttpProxy::HttpProxy(unsigned int bind_address, int port) {
    this->prepareMainSocket(bind_address, port);
    this->prepareSignalHandling();


//    auto* r = new HTTPResponse();
//    r->version = "HTTP/1.1";
//    r->statusCode = 200;
//    r->statusText = "OK";
//    r->addHeader("access-control-allow-origin", "*");
//    r->addHeader("x-frame-options", "SAMEORIGIN");
//    r->addHeader("x-xss-protection", "1; mode=block");
//    r->addHeader("x-content-type-options", "nosniff");
//    r->addHeader("referrer-policy", "strict-origin-when-cross-origin");
//    r->addHeader("content-type", "application/json; charset=utf-8");
//    r->addHeader("content-length", "4");
//    r->addHeader("date", "Sun, 12 Dec 2021 15:19:58 GMT");
//    r->addHeader("x-envoy-upstream-service-time", "2");
//    r->addHeader("vary", "Accept-Encoding");
//    r->addHeader("Via", "1.1 google");
//    r->data = "abcd";
//    r->inCache = true;
//    this->cache.emplace_back(r, "http://ipinfo.io/", 0);
}

void HttpProxy::run() {
    while (this->loop) {
        int poller = poll(this->sockets, MAX_CONNECTIONS, -1);
        if (poller == 0) {
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
    printf("Accepted new connection from %s:%d at poll slot %d, socket %d!\n", addr, ntohs(client.sin_port), pollSlot, con);
}

HTTPResponse* HttpProxy::queryCache(HTTPRequest *req) {
    for (cacheEntry& e : this->cache) {
        if (req->path == e.path) {
            return e.res;
        }
    }
    return nullptr;
}

void HttpProxy::closeConnection(int socket) {
    printf("Closing connection at socket %d\n", socket);
    delete this->connections[socket];
    this->connections.erase(socket);
    for (pollfd& cs : this->sockets) {
        if (cs.fd == socket) {
            cs.fd = -1;
            cs.events = 0;
            break;
        }
    }
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
    ssize_t bytes_read = read(event.fd, buf, BUF_SIZE);
    if (bytes_read > 0) {
        Connection *con = this->connections[event.fd];
        con->parser->read(std::string(buf, bytes_read));
        // handle next addr request
        if (con->getType() == Connection::SERVER) {

            if (!con->parser->entitiesToHandle.empty() && con->getStatus() == Connection::READY_TO_RECV) {
                if (con->req != nullptr) {
                    delete con->req; // delete old request
                }
                con->req = (HTTPRequest *)con->parser->entitiesToHandle[0];
                con->parser->entitiesToHandle.erase(con->parser->entitiesToHandle.begin()); // TODO: use something faster

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
                delete con->res;
                con->res = (HTTPResponse*)con->parser->entitiesToHandle[0];
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
        if (clientConnection->getStatus() == Connection::READY_TO_SEND) {
            if (clientConnection->getType() == Connection::CLIENT) {
                std::string req = clientConnection->req->toString();
                printf("%s\n", clientConnection->req->path.c_str());
                ssize_t bytes_sent = write(clientConnection->socket, req.c_str(), req.size());
                if (bytes_sent >= (long) req.size()) {
                    clientConnection->setStatus(Connection::READY_TO_RECV);
                }
            } else if (clientConnection->getType() == Connection::SERVER) {
                std::string res = clientConnection->res->toString();
                ssize_t bytes_sent = write(clientConnection->socket, res.c_str(), res.size());
                if (bytes_sent >= (long) res.size()) {
                    if (this->listeners.count(clientConnection->observable)) {
                        if (this->listeners[clientConnection->observable].size() == 1) {
                            this->removeListener(clientConnection->observable, clientConnection);
                            delete clientConnection->res;
                            clientConnection->res = nullptr;
                        } else {
                            this->removeListener(clientConnection->observable, clientConnection);
                        }
                    }
                    clientConnection->setStatus(Connection::READY_TO_RECV);
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
    int flag = connect(con, (sockaddr*)addr, sizeof(*addr));
    if (flag) {
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
