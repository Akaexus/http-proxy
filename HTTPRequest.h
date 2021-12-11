//
// Created by krzysztof on 10.12.2021.
//

#ifndef HTTP_PROXY_HTTPREQUEST_H
#define HTTP_PROXY_HTTPREQUEST_H


#include "HTTP.h"

class HTTPRequest : public HTTP {
    public:
//        enum methods {CONNECT, DELETE, GET, HEAD, OPTIONS, POST, PUT, TRACE};
//        methods method;
        std::string method;
        std::string path;
        std::string version;
};


#endif //HTTP_PROXY_HTTPREQUEST_H
