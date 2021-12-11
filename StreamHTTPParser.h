#ifndef HTTP_PROXY_STREAMHTTPPARSER_H
#define HTTP_PROXY_STREAMHTTPPARSER_H


#include <string>
#include "HTTP.h"
#include "HTTPRequest.h"

class StreamHTTPParser {

public:
    StreamHTTPParser();
    explicit StreamHTTPParser(HTTP::Type type);
    HTTPRequest* req = nullptr;
    HTTP::Type type;
    std::string buf;
    std::string tokenBuf;
    HTTPRequest* read(const std::string string);
    std::vector<HTTPRequest*> requestsToHandle;

protected:
    bool gotMethod = false;
    bool gotPath = false;
    bool gotHTTPVersion = false;
    bool gotAllHeaders = false;
    bool containsBody = false;
    bool gotBody = false;
    unsigned long contentLength = 0;
    HTTPRequest* produceRequest();
};


#endif //HTTP_PROXY_STREAMHTTPPARSER_H
