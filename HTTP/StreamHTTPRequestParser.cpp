#include "StreamHTTPRequestParser.h"



HTTPRequest* StreamHTTPRequestParser::read(const std::string string) {
    this->buf += string;
    while (!buf.empty()) {
        char current = buf[0];
        buf.erase(0, 1);
        // HTTP METHOD
        if (!this->gotMethod && current == ' ') {
            bool valid = false;
            for (auto& m : HTTP::methods) {
                if (m == tokenBuf) {
                    valid = true;
                    break;
                }
            }
            if (!valid) {
                throw std::runtime_error("Invalid HTTP method!");
            }
            this->req->method = tokenBuf;
            tokenBuf = "";
            this->gotMethod = true;
        } else if (!this->gotPath && current == ' ') { // PATH
            this->req->path = tokenBuf;
            tokenBuf = "";
            this->gotPath = true;
        } else if (!this->gotHTTPVersion && !tokenBuf.empty() && tokenBuf.back() == '\r' && current == '\n') { // HTTP VERSION
            tokenBuf.pop_back();
            this->req->version = tokenBuf;
            this->gotHTTPVersion = true;
            tokenBuf = "";
        } else if (!this->gotAllHeaders && tokenBuf == "\r" && current == '\n') { // end of headers
            this->gotAllHeaders = true;
            this->contentLength = this->req->contentLength();
            if (this->contentLength > 0) {
                this->containsBody = true;
            } else {
                this->produceRequest();
            }
        } else if (!this->gotAllHeaders && !tokenBuf.empty() && tokenBuf.back() == '\r' && current == '\n') { // header
            tokenBuf.pop_back();
            bool toValue = false;
            std::string header, value;
            for (long unsigned int i = 0; i < tokenBuf.size(); i++) {
                char letter = tokenBuf[i];
                if (!toValue && letter == ':' && tokenBuf[i + 1] == ' ') {
                    toValue = true;
                    i++;
                } else if (toValue) {
                    value += letter;
                } else {
                    header += letter;
                }
            }
            this->req->addHeader(header, value);
            tokenBuf = "";
        } else if (this->gotAllHeaders  && (!this->containsBody || (this->containsBody && tokenBuf.size() >= this->contentLength))) { // PRODUCE request
            if (this->containsBody) {
                tokenBuf.erase(0, 1);
                this->req->data = tokenBuf + current;
            }
            return this->produceRequest();
        } else {
            if (!this->gotMethod) {
                if (tokenBuf.size() > 7 || current == '\n') {
                    throw std::runtime_error("Bad HTTP method");
                }
            }
            if (this->gotMethod && !this->gotPath) {
                if (tokenBuf.size() > MAX_URI_LENGTH) {
                    throw std::runtime_error("Request URI too long");
                }
            }
            if (this->gotPath && !this->gotHTTPVersion) {
                if (tokenBuf.size() > 8) {
                    throw std::runtime_error("Invalid http method");
                }
            }
            if (this->gotHTTPVersion && !this->gotAllHeaders) {
                if (tokenBuf.size() > MAX_HEADER_LENGTH) {
                    throw std::runtime_error("Header too long");
                }
            }
            tokenBuf += current;
        }
    }
    return nullptr;
}

StreamHTTPRequestParser::StreamHTTPRequestParser() {
    this->req = new HTTPRequest();
};

HTTPRequest* StreamHTTPRequestParser::produceRequest() {
    this->entitiesToHandle.push_back(this->req);
    HTTPRequest* request = this->req;
    this->req = new HTTPRequest();
    this->tokenBuf = "";
    this->gotMethod = false;
    this->gotPath = false;
    this->gotHTTPVersion = false;
    this->gotAllHeaders = false;
    this->containsBody = false;
    this->gotBody = false;
    this->contentLength = 0;
    return request;
}

StreamHTTPRequestParser::~StreamHTTPRequestParser() {
    delete this->req;
}
