#include <cstdio>
#include "StreamHTTPParser.h"



HTTPRequest* StreamHTTPParser::read(const std::string string) {

    this->buf += string;
    while (!buf.empty()) {
        char current = buf[0];
        buf.erase(0, 1);
        // HTTP METHOD
        if (!this->gotMethod && current == ' ') {
            this->req->method = tokenBuf;
            tokenBuf = "";
            this->gotMethod = true;
        } else if (!this->gotPath && current == ' ') { // PATH
            this->req->path = tokenBuf;
            tokenBuf = "";
            this->gotPath = true;
        } else if (!this->gotHTTPVersion && tokenBuf.back() == '\r' && current == '\n') { // HTTP VERSION
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
        } else if (!this->gotAllHeaders && tokenBuf.back() == '\r' && current == '\n') { // header
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
            tokenBuf += current;
        }
    }
    return nullptr;
}

StreamHTTPParser::StreamHTTPParser(HTTP::Type type) {
    this->type = type;
    this->req = new HTTPRequest();
}

StreamHTTPParser::StreamHTTPParser() = default;

HTTPRequest* StreamHTTPParser::produceRequest() {
    this->requestsToHandle.push_back(this->req);
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
