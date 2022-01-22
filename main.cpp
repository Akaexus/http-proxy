#include <cstdio>
#include <netinet/in.h>
#include <cstring>
#include "HttpProxy.h"

int main(int argc, char* argv[]) {
    int port = 8080;
    int verbose_level = 1;
    for (int i = 0; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0) {
            printf("Usage: ./http_proxy [options...]\n -h - help\n -p [PORT] - bind port\n -v [LEVEL] - set verbose level (0 to 1) (default 1)\n");
            exit(0);
        }
        if (strcmp(argv[i], "-p") == 0) {
            port = std::stoi(argv[i + 1]);
            if (port <= 0) {
                port = 8080;
            }
        }
        if (strcmp(argv[i], "-v") == 0) {
            verbose_level = std::stoi(argv[i + 1]);
            if (verbose_level < 0 || verbose_level > 1) {
                verbose_level = 1;
            }
        }
    }
    HttpProxy proxy = HttpProxy(INADDR_ANY, port, verbose_level);
    printf("Server started at port %d!\n", port);
    proxy.run();
}
