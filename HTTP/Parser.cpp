#include "Parser.h"

Parser::~Parser() {
    for(auto r : this->entitiesToHandle) {
        delete r;
    }
}
