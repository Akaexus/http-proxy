#ifndef HTTP_PROXY_PARSER_H
#define HTTP_PROXY_PARSER_H


#include "HTTP.h"

class Parser {
    public:
        virtual HTTP* read(std::string string) = 0;
        std::vector<HTTP*> entitiesToHandle;
        virtual ~Parser();
};


#endif
