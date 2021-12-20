#ifndef HTTP_PROXY_HEADER_H
#define HTTP_PROXY_HEADER_H


#include <string>

class Header {
public:
    Header(std::string header, std::string value);
    std::string header;
    std::string value;
};


#endif //HTTP_PROXY_HEADER_H
