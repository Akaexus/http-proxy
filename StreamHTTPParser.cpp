#include <cstdio>
#include "StreamHTTPParser.h"



void StreamHTTPParser::read(const std::string string) {
    this->buf += string;
    while (!buf.empty()) {
        char current = buf[0];
        buf.erase(0, 1);
        // HTTP METHOD
        if (!this->gotMethod && current == ' ') {
            this->req.method = tokenBuf;
            tokenBuf = "";
            this->gotMethod = true;
            printf("Method: %s\n", this->req.method.c_str());
        } else if (!this->gotPath && current == ' ') { // PATH
            this->req.path = tokenBuf;
            tokenBuf = "";
            this->gotPath = true;
            printf("Path: %s\n", this->req.path.c_str());
        } else if (!this->gotHTTPVersion && current == '\n') { // HTTP VERSION

        } else {
            tokenBuf += current;
        }
    }

}

StreamHTTPParser::StreamHTTPParser(HTTP::Type type) {
    this->type = type;
}

StreamHTTPParser::StreamHTTPParser() {

}
