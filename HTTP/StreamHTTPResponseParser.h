#ifndef HTTP_PROXY_STREAMHTTPRESPONSEPARSER_H
#define HTTP_PROXY_STREAMHTTPRESPONSEPARSER_H


#include "Parser.h"
#include "HTTPResponse.h"

class StreamHTTPResponseParser : public Parser {
    public:
        StreamHTTPResponseParser();
        HTTPResponse* res = nullptr;
        std::string buf;
        std::string tokenBuf;
        HTTPResponse* read(std::string string) override;
        ~StreamHTTPResponseParser() override;

    protected:
        bool gotHTTPVersion = false;
        bool gotStatusCode = false;
        bool gotStatusText = false;
        bool gotAllHeaders = false;
        bool containsBody = false;
        bool gotBody = false;
        unsigned long contentLength = 0;
        HTTPResponse* produceRequest();
};


#endif
