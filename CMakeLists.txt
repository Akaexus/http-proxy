cmake_minimum_required(VERSION 3.20)
project(http_proxy)

set(CMAKE_CXX_STANDARD 20)
add_compile_options(-Wall -Wextra -pedantic -Werror)

add_executable(http_proxy main.cpp)
