#include <cstdio>
#include <netinet/in.h>
#include "HttpProxy.h"

int main() {
    HttpProxy proxy = HttpProxy(INADDR_ANY, 8080);
    printf("Server started!\n");
    proxy.run();
}
 