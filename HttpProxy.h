#ifndef HTTP_PROXY_HTTPPROXY_H
#define HTTP_PROXY_HTTPPROXY_H


#include <sys/poll.h>
#include <map>
#include <utility>
#include "HTTP/StreamHTTPRequestParser.h"
#include "HTTP/HTTPRequest.h"
#include "HTTP/HTTPResponse.h"
#include "Connection.h"
#include <unistd.h>
#include <ctime>


#define MAX_CONNECTIONS 1024
#define BUF_SIZE 1024

class HttpProxy {
    public:
        HttpProxy(unsigned int address, int port, int verbose_level);
        ~HttpProxy();
        void run();
        void stop();
        int loop = true;
        int verbosity;
        // HTTP Response cache
        struct cacheEntry {
            HTTPResponse* res;
            std::string path;
            time_t expireAt;
            cacheEntry(HTTPResponse* r, std::string p, long e);
            ~cacheEntry();
        };
        std::vector<cacheEntry> cache;
        HTTPResponse* queryCache(HTTPRequest* req);
        void addToCache(HTTPResponse* res, std::string path, long age);


protected:
    int main_socket;
    int sfd; // signal file descriptor
    struct sockaddr_in listen_address;
    pollfd sockets[MAX_CONNECTIONS];
    std::map<int, Connection*> connections;
    std::map<int, HTTPResponse*> proxy_responses;
    // SETUP PROXY
    void prepareSignalHandling();
    void prepareMainSocket(unsigned int bind_address, int port);
    void prepareProxyResponses();
    void cleanupProxyResponses();

    std::map<Connection*, std::vector<Connection*>> listeners;
    void addListener(Connection* observable, Connection* observer); // observer listens to events on observable
    std::vector<Connection*> getListeners(Connection* observable);
    void removeListener(Connection* observable, Connection* observer);

    // HANDLERS
    void handleIncomingConnection();
    void handleIncomingData(pollfd event);
    void handleOutgoingData(pollfd event);
    void handleSignal(pollfd event);
    // UTILITIES
    int getEmptyPollSlot();
    void registerNewConnection(int pollSlot);
    void closeConnection(int socket);

    void makeHTTPRequest(Connection *pConnection);
    void cacheIfCan(HTTPResponse* res, HTTPRequest* req);

    static long stringToEpoch(std::string t);
};


#endif //HTTP_PROXY_HTTPPROXY_H
