// SERVER TCP
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
int main() {

    int s = socket(AF_INET, SOCK_STREAM, 0);

    const int one = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));


    struct sockaddr_in sa;
    sa.sin_family = AF_INET;
    sa.sin_port = htons(8080);
    sa.sin_addr = {INADDR_ANY};

    bind(s, (struct sockaddr*)&sa, sizeof(sa));

    listen(s, 16);
    struct sockaddr_in css;
    socklen_t cssl = sizeof(css);
    printf("Server started!\n");
    while (1) {
        // handle incoming connection
        int cs = accept(s, (struct sockaddr*)&css, &cssl);
        char addr[255] = {0};
        inet_ntop(AF_INET, &css.sin_addr, addr, sizeof(addr));
        printf("Connection from %s:%d\n", addr, ntohs(css.sin_port));
        char buf[] = "hello";
        write(cs, buf, sizeof(buf));

        shutdown(cs, SHUT_RDWR);
        close(cs);
    }

    close(s);
}
 