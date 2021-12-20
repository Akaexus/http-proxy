#include "HTTPResponse.h"

std::string HTTPResponse::toString() {
    std::string response = this->version + " " + std::to_string(this->statusCode) + " " + this->statusText + "\r\n";
    for (Header* h : this->headers) {
        response += h->header + ": " + h->value + "\r\n";
    }
    response += "\r\n";
    if (!this->data.empty()) {
        response += this->data;
    }

    return response;
}
