#ifndef HTTP_PROXY_STREAMHTTPREQUESTPARSER_H
#define HTTP_PROXY_STREAMHTTPREQUESTPARSER_H

#include <string>
#include "HTTP.h"
#include "HTTPRequest.h"
#include "Parser.h"

#define MAX_URI_LENGTH 2048
#define MAX_HEADER_LENGTH 8192

class StreamHTTPRequestParser : public Parser {
    public:
        StreamHTTPRequestParser();
        HTTPRequest* req = nullptr;
        std::string buf;
        std::string tokenBuf;
        HTTPRequest* read(std::string string) override;
        ~StreamHTTPRequestParser() override;

    protected:
        bool gotMethod = false;
        bool gotPath = false;
        bool gotHTTPVersion = false;
        bool gotAllHeaders = false;
        bool containsBody = false;
        unsigned long contentLength = 0;
        HTTPRequest* produceRequest();
};

#endif
