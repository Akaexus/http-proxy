#include "Connection.h"
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include "HTTP/StreamHTTPParser.h"



Connection::Connection(int s, struct sockaddr_in address, int slot) {
    this->socket = s;
    this->client = address;
    this->poll_slot = slot;
    this->parser = new StreamHTTPParser();
}

Connection::~Connection() {
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