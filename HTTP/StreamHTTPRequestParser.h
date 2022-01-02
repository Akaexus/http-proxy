#ifndef HTTP_PROXY_STREAMHTTPREQUESTPARSER_H
#define HTTP_PROXY_STREAMHTTPREQUESTPARSER_H

#include <string>
#include "HTTP.h"
#include "HTTPRequest.h"
#include "Parser.h"

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
        bool gotBody = false;
        unsigned long contentLength = 0;
        HTTPRequest* produceRequest();
};

#endif
