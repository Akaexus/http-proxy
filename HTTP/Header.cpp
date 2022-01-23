#include "Header.h"

#include <utility>

Header::Header(std::string h, std::string v) {
    this->header = std::move(h);
    this->value = std::move(v);
}
