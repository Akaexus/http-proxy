#ifndef HTTP_PROXY_STREAMHTTPPARSER_H
#define HTTP_PROXY_STREAMHTTPPARSER_H


#include <string>
#include "HTTP.h"
#include "HTTPRequest.h"

class StreamHTTPParser {

public:
    StreamHTTPParser();
    explicit StreamHTTPParser(HTTP::Type type);
    HTTPRequest req;
    HTTP::Type type;
    std::string buf;
    std::string tokenBuf;
    void read(const std::string string);

protected:
    bool gotMethod = false;
    bool gotPath = false;
    bool gotHTTPVersion = false;
};


#endif //HTTP_PROXY_STREAMHTTPPARSER_H
