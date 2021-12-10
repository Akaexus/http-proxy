//
// Created by krzysztof on 10.12.2021.
//

#ifndef HTTP_PROXY_HTTP_H
#define HTTP_PROXY_HTTP_H


#include <vector>
#include <string>
#include "Header.h"

class HTTP {
    public:
        enum Type {Request, Response, NA};
        std::vector<Header> headers;
        std::string data;
};


#endif //HTTP_PROXY_HTTP_H
