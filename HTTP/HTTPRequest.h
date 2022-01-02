#ifndef HTTP_PROXY_HTTPREQUEST_H
#define HTTP_PROXY_HTTPREQUEST_H


#include "HTTP.h"

class HTTPRequest : public HTTP {
    public:
//        enum methods {CONNECT, DELETE, GET, HEAD, OPTIONS, POST, PUT, TRACE};
//        methods method;
        std::string method;
        std::string path;
        std::string toString() override;
};


#endif //HTTP_PROXY_HTTPREQUEST_H
