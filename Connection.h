//
// Created by krzysztof on 20.12.2021.
//

#ifndef HTTP_PROXY_CONNECTION_H
#define HTTP_PROXY_CONNECTION_H


#include <netinet/in.h>
#include "HTTP/StreamHTTPParser.h"
#include "HTTP/HTTPResponse.h"

class Connection {
    public:
        int socket = -1;
        enum connectionStatus {
            HANDLED,
            READY_TO_SEND_RESPONSE,
        };
        StreamHTTPParser *parser;
        struct sockaddr_in client{};
        int poll_slot;
        HTTPRequest* requestToHandle = nullptr;
        HTTPResponse* res = nullptr;
        connectionStatus status = HANDLED;
        Connection(int s, struct sockaddr_in address, int slot);
        ~Connection();
};


#endif //HTTP_PROXY_CONNECTION_H
