#include "StreamHTTPResponseParser.h"

StreamHTTPResponseParser::StreamHTTPResponseParser() {
    this->res = new HTTPResponse();
}

StreamHTTPResponseParser::~StreamHTTPResponseParser() {
    delete this->res;
}

HTTPResponse *StreamHTTPResponseParser::read(std::string string) {
    this->buf += string;
    while (!buf.empty()) {
        char current = buf[0];
        buf.erase(0, 1);
        // HTTP VERSION
        if (!this->gotHTTPVersion && !tokenBuf.empty() && current == ' ') { // HTTP VERSION
            this->res->version = tokenBuf;
            this->gotHTTPVersion = true;
            tokenBuf = "";
        } else if (this->gotHTTPVersion && !this->gotStatusCode && !tokenBuf.empty() && current == ' ') {
            this->res->statusCode = strtol(tokenBuf.c_str(), nullptr, 10);
            this->gotStatusCode = true;
            tokenBuf = "";
        } else if (this->gotStatusCode && !this->gotStatusText && !tokenBuf.empty() && tokenBuf.back() == '\r' && current == '\n') {
            tokenBuf.pop_back();
            this->res->statusText = tokenBuf;
            this->gotStatusText = true;
            tokenBuf = "";
        } else if (!this->gotAllHeaders && tokenBuf == "\r" && current == '\n') { // end of headers
            this->gotAllHeaders = true;
            this->contentLength = this->res->contentLength();
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
            this->res->addHeader(header, value);
            tokenBuf = "";
        } else if (this->gotAllHeaders  && (!this->containsBody || (this->containsBody && tokenBuf.size() >= this->contentLength))) { // PRODUCE request
            if (this->containsBody) {
                tokenBuf.erase(0, 1);
                this->res->data = tokenBuf + current;
            }
            return this->produceRequest();
        }
        else {
            tokenBuf += current;
        }
    }
    return this->res;
}

HTTPResponse* StreamHTTPResponseParser::produceRequest() {
    this->entitiesToHandle.push_back(this->res);
    HTTPResponse* response = this->res;
    this->res = new HTTPResponse();
    this->gotHTTPVersion = false;
    this->gotStatusCode = false;
    this->gotStatusText = false;
    this->gotAllHeaders = false;
    this->containsBody = false;
    this->contentLength = 0;
    return response;
}
