#ifndef HTTP_PROXY_URI_H
#define HTTP_PROXY_URI_H


#include <string>
#include <algorithm>    // find

// https://stackoverflow.com/posts/11044337
struct URI {
    public:
        std::string QueryString, Path, Protocol, Host, Port;
        static URI Parse(const std::string &uri);
};



#endif
