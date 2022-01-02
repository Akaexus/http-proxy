#include "Connection.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "HTTP/StreamHTTPRequestParser.h"
#include "HTTP/StreamHTTPResponseParser.h"


Connection::Connection(connectionType t, int s, struct sockaddr_in address, pollfd* ps) {
    this->socket = s;
    this->addr = address;
    this->pollSlot = ps;
    this->setType(t);
    if (this->getType() == SERVER) {
        this->parser = new StreamHTTPRequestParser();
        this->setStatus(READY_TO_RECV);
    } else {
        this->parser = new StreamHTTPResponseParser();
        this->setStatus(READY_TO_SEND);
    }
}

Connection::~Connection() {
    if (this->socket > -1) {
        shutdown(this->socket, SHUT_RDWR);
        close(socket);
    }
    if (this->getType() != CLIENT) {
        delete this->req;
    }
    if (res != nullptr && !res->inCache) {
        delete this->res;
    }
    delete this->parser;
}

void Connection::setStatus(Connection::connectionStatus s) {
    this->status = s;
    switch(s) {
        case READY_TO_RECV:
            this->pollSlot->events = POLLIN;
            break;
        case READY_TO_SEND:
            this->pollSlot->events = POLLOUT ;
            break;
        case WAITING:
            this->pollSlot->events = 0;
            break;
    }
}

Connection::connectionStatus Connection::getStatus() {
    return this->status;
}

void Connection::setType(connectionType t) {
    this->type = t;
}

Connection::connectionType Connection::getType() {
    return this->type;
}
