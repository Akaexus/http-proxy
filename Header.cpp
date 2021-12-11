//
// Created by krzysztof on 10.12.2021.
//

#include "Header.h"

#include <utility>

Header::Header(std::string header, std::string value) {
    this->header = std::move(header);
    this->value = std::move(value);
}
