#include <cstdio>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>
#include "StreamHTTPParser.h"
#include "HttpProxy.h"
int loop = 1;
int main() {

    HttpProxy proxy = HttpProxy(INADDR_ANY, 8080);
    printf("Server started!\n");
    proxy.run();
//        while (loop) {
//            int bytes_read = read(cs, buf, sizeof(buf));
//            parser.read(buf, bytes_read);
//        }
//        shutdown(cs, SHUT_RDWR);
//        close(cs);
//    }
//
//    close(s);
}
 