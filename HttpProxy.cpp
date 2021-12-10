#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstdio>
#include <cerrno>
#include <cstring>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include "StreamHTTPParser.h"
#include "HttpProxy.h"



HttpProxy::HttpProxy(unsigned int bind_address, int port) {
    this->main_socket = socket(AF_INET, SOCK_STREAM, 0);

    int one = 1;
    setsockopt(this->main_socket, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

    this->listen_address.sin_family = AF_INET;
    this->listen_address.sin_port = htons(port);
    this->listen_address.sin_addr = {bind_address};

    int bind_flag = bind(this->main_socket, (struct sockaddr*)&this->listen_address, sizeof(this->listen_address));

    if (bind_flag < 0) {
        perror("Cannot bind to the socket");
    }

    int listen_flag = listen(this->main_socket, 16);

    if (listen_flag < 0) {
        perror("Cannot listen to the socket");
    }
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        this->sockets[i].fd = -1;
        this->sockets[i].events = 0;
    }
}

void HttpProxy::run() {
    this->sockets[0].fd = this->main_socket;
    this->sockets[0].events = POLLIN;



    while (this->loop) {
        int poller = poll(this->sockets, MAX_CONNECTIONS, -1);
        if (poller == 0) {
            perror("Cannot poll");
        }

        for (pollfd &event: this->sockets) {
            // accept new connections
            if (event.fd == this->main_socket && event.revents & POLLIN) {
                int emptySlot = this->getEmptyPollSlot();
                if (emptySlot >= 0) {
                    this->registerNewConnection(emptySlot);
                }
            }
            // communication
            if (event.fd != this->main_socket && event.revents & POLLIN) {
                char buf[BUF_SIZE] = {0};
                ssize_t bytes_read = read(event.fd, buf, BUF_SIZE);
                if (bytes_read > 0) {
                    this->connections[event.fd].parser.read(std::string(buf, bytes_read));
                    printf("Read %zd bytes from socket %d at poll slot %d\n", bytes_read, event.fd,
                           this->connections[event.fd].poll_slot);
                    printf("%s\n", buf);
                }
            }
        }
    }
}

int HttpProxy::getEmptyPollSlot() {
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (this->sockets[i].fd == -1) {
            return i;
        }
    }
    return -1;
}

void HttpProxy::registerNewConnection(int pollSlot) {
    struct sockaddr_in client;
    socklen_t size = sizeof(client);
    int con = accept(this->main_socket, (struct sockaddr *) &client, &size);

    this->connections[con] = {
            con,
            StreamHTTPParser(HTTP::Request),
            client,
            pollSlot
    };
    this->sockets[pollSlot].fd = con;
    this->sockets[pollSlot].events = POLLIN;
    char addr[255] = {0};
    inet_ntop(AF_INET, &client.sin_addr, addr, sizeof(addr));
    printf("Accepted new connection from %s:%d at poll slot %d!\n", addr, ntohs(client.sin_port), pollSlot);
}
