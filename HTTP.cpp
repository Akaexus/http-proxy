#include <algorithm>
#include <cctype>
#include <iostream>
#include "HTTP.h"

void HTTP::addHeader(const std::string& header, const std::string& value) {
    this->headers.emplace_back(header, value);
}

unsigned long HTTP::contentLength() {
    Header* h = this->getHeader("Content-Length");
    if (h != nullptr) {
        try {
            long contentLength = std::stol(h->value);
            return contentLength < 0 ? 0 : contentLength;
        } catch (std::invalid_argument &err) {
            return 0;
        } catch (std::out_of_range &err) {
            return 0;
        }
    }
    return 0;
}

Header* HTTP::getHeader(std::string headerName) {
    headerName = HTTP::tolower(headerName);
    for (Header & h : this->headers) {
        if (HTTP::tolower(h.header) == headerName) {
            return &h;
        }
    }
    return nullptr;
}

std::string HTTP::tolower(std::string in) {
    std::locale loc;
    for(auto &elem : in) {
        elem = std::tolower(elem, loc);
    }
    return in;
}
