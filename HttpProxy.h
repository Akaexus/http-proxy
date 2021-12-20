#ifndef HTTP_PROXY_HTTPPROXY_H
#define HTTP_PROXY_HTTPPROXY_H


#include <sys/poll.h>
#include <map>
#include <utility>
#include "HTTP/StreamHTTPParser.h"
#include "HTTP/HTTPRequest.h"
#include "HTTP/HTTPResponse.h"
#include "Connection.h"
#include <unistd.h>


#define MAX_CONNECTIONS 10
#define BUF_SIZE 1024

class HttpProxy {
    public:
        HttpProxy(unsigned int address, int port);
        ~HttpProxy();
        void run();
        void stop();
        int loop = true;
        // HTTP Response cache
        struct cacheEntry {
            HTTPResponse* res;
            std::string path;
            long expireAt;
            cacheEntry(HTTPResponse* r, std::string p, long e);
            ~cacheEntry();
        };
        std::vector<cacheEntry> cache;

        HTTPResponse* queryCache(HTTPRequest* req);


protected:
    int main_socket;
    int sfd; // signal file descriptor
    struct sockaddr_in listen_address;
    pollfd sockets[MAX_CONNECTIONS];
    std::map<int, Connection*> connections;
    // SETUP PROXY
    void prepareSignalHandling();
    void prepareMainSocket(unsigned int bind_address, int port);

    // HANDLERS
    void handleIncomingConnection();
    void handleIncomingData(pollfd event);
    void handleOutgoingData(pollfd event);
    void handleSignal(pollfd event);
    // UTILITIES
    int getEmptyPollSlot();
    void registerNewConnection(int pollSlot);
    void closeConnection(int socket);
};


#endif //HTTP_PROXY_HTTPPROXY_H
