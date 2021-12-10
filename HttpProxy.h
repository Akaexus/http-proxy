#ifndef HTTP_PROXY_HTTPPROXY_H
#define HTTP_PROXY_HTTPPROXY_H


#include <sys/poll.h>
#include <map>
#include "StreamHTTPParser.h"

#define MAX_CONNECTIONS 1024
#define BUF_SIZE 1024

class HttpProxy {
    public:
        HttpProxy(unsigned int address, int port);
        void run();
        int loop = true;
        struct connection {
            int socket{};
            StreamHTTPParser parser;
            struct sockaddr_in client{};
            int poll_slot{};
        };

protected:
        int main_socket;
        struct sockaddr_in listen_address;
        pollfd sockets[MAX_CONNECTIONS];
        std::map<int, connection> connections;

    int getEmptyPollSlot();

    void registerNewConnection(int pollSlot);
};


#endif //HTTP_PROXY_HTTPPROXY_H
