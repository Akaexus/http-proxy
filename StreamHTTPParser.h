#ifndef HTTP_PROXY_STREAMHTTPPARSER_H
#define HTTP_PROXY_STREAMHTTPPARSER_H


#include <string>
#include "HTTP.h"
#include "HTTPRequest.h"

class StreamHTTPParser {

public:
    StreamHTTPParser();
    HTTPRequest* req = nullptr;
    std::string buf;
    std::string tokenBuf;
    HTTPRequest* read(const std::string string);
    std::vector<HTTPRequest*> requestsToHandle;
    ~StreamHTTPParser();

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
