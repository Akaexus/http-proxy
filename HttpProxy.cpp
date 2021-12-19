#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include "StreamHTTPParser.h"
#include "HttpProxy.h"
#include <sys/signalfd.h>
#include <csignal>


HttpProxy::connection::connection(int s, struct sockaddr_in address, int slot) {
    this->socket = s;
    this->client = address;
    this->poll_slot = slot;
    this->parser = new StreamHTTPParser();
}

HttpProxy::connection::~connection() {
    if (this->socket > -1) {
        shutdown(this->socket, SHUT_RDWR);
        close(socket);
    }
    delete this->requestToHandle;
    if (res != nullptr && !res->inCache) {
        delete this->res;
    }
    delete this->parser;
}

HttpProxy::HttpProxy(unsigned int bind_address, int port) {
    this->prepareMainSocket(bind_address, port);
    this->prepareSignalHandling();


    auto* r = new HTTPResponse();
    r->version = "HTTP/1.1";
    r->statusCode = 200;
    r->statusText = "OK";
    r->addHeader("access-control-allow-origin", "*");
    r->addHeader("x-frame-options", "SAMEORIGIN");
    r->addHeader("x-xss-protection", "1; mode=block");
    r->addHeader("x-content-type-options", "nosniff");
    r->addHeader("referrer-policy", "strict-origin-when-cross-origin");
    r->addHeader("content-type", "application/json; charset=utf-8");
    r->addHeader("content-length", "4");
    r->addHeader("date", "Sun, 12 Dec 2021 15:19:58 GMT");
    r->addHeader("x-envoy-upstream-service-time", "2");
    r->addHeader("vary", "Accept-Encoding");
    r->addHeader("Via", "1.1 google");
    r->data = "abcd";
    r->inCache = true;
    this->cache.emplace_back(r, "http://ipinfo.io/", 0);
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

    this->connections[con] = new HttpProxy::connection(
            con,
            client,
            pollSlot
    );
    this->sockets[pollSlot].fd = con;
    this->sockets[pollSlot].events = POLLIN | POLLOUT;
    char addr[255] = {0};
    inet_ntop(AF_INET, &client.sin_addr, addr, sizeof(addr));
    printf("Accepted new connection from %s:%d at poll slot %d!\n", addr, ntohs(client.sin_port), pollSlot);
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
        HttpProxy::connection *clientConnection = this->connections[event.fd];
        clientConnection->parser->read(std::string(buf, bytes_read));
        // handle next client request
        if (!clientConnection->parser->requestsToHandle.empty() && clientConnection->status == HANDLED) {
            delete clientConnection->requestToHandle; // delete old request
            clientConnection->requestToHandle = clientConnection->parser->requestsToHandle[0]; // TODO: use something faster
            clientConnection->parser->requestsToHandle.erase(clientConnection->parser->requestsToHandle.begin());

            clientConnection->res = queryCache(clientConnection->requestToHandle);
            clientConnection->status = READY_TO_SEND_RESPONSE;
        }
    } else {
        this->closeConnection(event.fd);
    }
}

void HttpProxy::handleOutgoingData(pollfd event) {
    if (this->connections.count(event.fd)) {
        HttpProxy::connection *clientConnection = this->connections[event.fd];
        if (clientConnection->status == READY_TO_SEND_RESPONSE) {
            std::string response = clientConnection->res->toString();
            ssize_t bytes_sent = write(clientConnection->socket, response.c_str(), response.size());
            if (bytes_sent >= (long) response.size()) {
                clientConnection->status = HANDLED;
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
    printf("SIGNAL %d\n", siginfo.ssi_signo);
    this->stop();
}
