#ifndef HTTP_PROXY_HTTPRESPONSE_H
#define HTTP_PROXY_HTTPRESPONSE_H

#include "HTTP.h"
#include <string>

class HTTPResponse : public HTTP  {
    public:
        unsigned short statusCode{};
        std::string statusText{};
        std::string toString() override;
        bool inCache = false;

};


#endif //HTTP_PROXY_HTTPRESPONSE_H
