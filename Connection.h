#ifndef HTTP_PROXY_CONNECTION_H
#define HTTP_PROXY_CONNECTION_H


#include <netinet/in.h>
#include <sys/poll.h>
#include "HTTP/StreamHTTPRequestParser.h"
#include "HTTP/HTTPResponse.h"

class Connection {
    public:
        int socket = -1;
        enum connectionType {
            CLIENT, // proxy -> server
            SERVER // client -> proxy
        };
        enum connectionStatus {
            READY_TO_RECV,
            READY_TO_SEND,
            WAITING
        };
        Parser *parser;
        struct sockaddr_in addr{};
        pollfd* pollSlot;
        HTTPRequest* req = nullptr;
        HTTPResponse* res = nullptr;
        Connection* observable = nullptr;
        Connection(connectionType t, int s, struct sockaddr_in address, pollfd* ps);
        ~Connection();
        void setStatus(Connection::connectionStatus s);
        connectionStatus getStatus();
        void setType(connectionType t);
        connectionType getType();


    protected:
        connectionStatus status = READY_TO_RECV;
        connectionType type = SERVER;
};


#endif //HTTP_PROXY_CONNECTION_H
