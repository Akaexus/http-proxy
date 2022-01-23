#ifndef HTTP_PROXY_HTTP_H
#define HTTP_PROXY_HTTP_H


#include <vector>
#include <string>
#include <iostream>
#include "Header.h"

class HTTP {
    public:
        std::vector<Header*> headers;
        void addHeader(const std::string& header, const std::string& value);
        void removeHeader(const std::string& header);
        Header* getHeader(std::string headerName);
        virtual std::string toString() = 0;
        virtual ~HTTP();
    unsigned long contentLength();
        std::string data;
        std::string version;
        static const inline std::string methods[] = {
                "GET",
                "HEAD",
                "POST",
                "PUT",
                "DELETE",
                "CONNECT",
                "OPTIONS",
                "TRACE",
                "PATCH",
        };
//    protected:
        static std::string tolower(std::string in);
        static std::string trim(std::string &str);
};


#endif //HTTP_PROXY_HTTP_H
