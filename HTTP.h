#ifndef HTTP_PROXY_HTTP_H
#define HTTP_PROXY_HTTP_H


#include <vector>
#include <string>
#include <iostream>
#include "Header.h"

class HTTP {
    public:
        std::vector<Header*> headers;
        void addHeader(const std::string header, const std::string value);
        Header* getHeader(std::string headerName);
    unsigned long contentLength();
        std::string data;
        std::string version;
        std::string body;
//    protected:
        static std::string tolower(std::string in);
};


#endif //HTTP_PROXY_HTTP_H
