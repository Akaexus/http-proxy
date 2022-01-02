#include "HTTPRequest.h"

std::string HTTPRequest::toString() {
    std::string request = this->method + " " + this->path + " " + this->version + "\r\n";
    for (Header* h : this->headers) {
        request += h->header + ": " + h->value + "\r\n";
    }
    request += "\r\n";
    if (!this->data.empty()) {
        request += this->data;
    }
    return request;
}
