cmake_minimum_required(VERSION 3.20)
project(http_proxy)

set(CMAKE_CXX_STANDARD 20)
add_compile_options(-Wall -Wextra -pedantic -Werror)

add_executable(http_proxy main.cpp HttpProxy.cpp HttpProxy.h HTTP/HTTP.cpp HTTP/HTTP.h HTTP/Header.cpp HTTP/Header.h HTTP/StreamHTTPParser.cpp HTTP/StreamHTTPParser.h HTTP/HTTPRequest.cpp HTTP/HTTPRequest.h HTTP/HTTPResponse.cpp HTTP/HTTPResponse.h Connection.cpp Connection.h)
