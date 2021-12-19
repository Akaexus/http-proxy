#ifndef HTTP_PROXY_HTTPPROXY_H
#define HTTP_PROXY_HTTPPROXY_H


#include <sys/poll.h>
#include <map>
#include <utility>
#include "StreamHTTPParser.h"
#include "HTTPRequest.h"
#include "HTTPResponse.h"
#include <unistd.h>


#define MAX_CONNECTIONS 10
#define BUF_SIZE 1024

class HttpProxy {
    public:
        HttpProxy(unsigned int address, int port);
        void run();
        void stop();
        int loop = true;
        // HTTP Response cache
        struct cacheEntry {
            HTTPResponse* res;
            std::string path;
            long expireAt;
            cacheEntry(HTTPResponse* r, std::string p, long e) {
                this->res = r;
                this->path = std::move(p);
                this->expireAt = e;
            }
            ~cacheEntry() {
                delete this->res;
            }
        };
        std::vector<cacheEntry> cache;

        HTTPResponse* queryCache(HTTPRequest* req);

        // CLIENT CONNECTIONS
        enum connectionStatus {
            HANDLED,
            READY_TO_SEND_RESPONSE,
        };
        struct connection {
            int socket = -1;
            StreamHTTPParser *parser;
            struct sockaddr_in client{};
            int poll_slot;
            HTTPRequest* requestToHandle = nullptr;
            HTTPResponse* res = nullptr;
            connectionStatus status = HANDLED;
            connection(int s, struct sockaddr_in address, int slot) {
                this->socket = s;
                this->client = address;
                this->poll_slot = slot;
                this->parser = new StreamHTTPParser();
            }
            ~connection() {
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

        };

protected:
        int main_socket;
        int sfd; // signal file descriptor
        struct sockaddr_in listen_address;
        pollfd sockets[MAX_CONNECTIONS];
        std::map<int, connection*> connections;



    int getEmptyPollSlot();
    void registerNewConnection(int pollSlot);
    void closeConnection(int socket);
};


#endif //HTTP_PROXY_HTTPPROXY_H
