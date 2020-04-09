#include <btree.h>
#include <command.h>
#include <networking.h>
#include <string>
#include <iostream>
void recv_n(int fd, char *buf, const int size) {
    int received = 0;
    while (received < size) {
        int chunk = recv(fd, buf, size - received, 0);
        if (chunk == -1) {
            perror("recv");
            return;
        }
        received += chunk;
        buf += chunk;
    }
}

void send_n(int fd, const char *buf, const int size) {
    int sent = 0;
    while (sent < size) {
        int chunk = send(fd, buf, size - sent, 0);
        if (chunk == -1) {
            perror("send");
            return;
        }
        sent += chunk;
        buf += chunk;
    }
}

int connect_to(const char *host, const char *port) {
    struct addrinfo hints, *servinfo, *p;
    int rv, fd;
    char s[INET6_ADDRSTRLEN];
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(host, port, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket error");
            continue;
        }

        if (connect(fd, p->ai_addr, p->ai_addrlen) == -1) {
            close(fd);
            perror("client: connect error");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        exit(2);
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)&p->ai_addr), s,
              sizeof s);
    fprintf(stdout, "client: connected to %s:%s\n", s, port);
    freeaddrinfo(servinfo);
    return fd;
}

Value Get(int connfd, Key key) {
    char getCommand[9] = {GET};
    memcpy(getCommand + 1, &key, 8);
    send_n(connfd, getCommand, sizeof getCommand);
    char reply[257] = {0};
    recv_n(connfd, reply, sizeof reply);
    Value value;
    memcpy(value.bytes, reply + 1, 256);
    return value;
}

bool Delete(int connfd, Key key) {
    char deleteCommand[9] = {DELETE};
    memcpy(deleteCommand + 1, &key, 8);
    send_n(connfd, deleteCommand, sizeof deleteCommand);
    char reply;
    recv_n(connfd, &reply, 1);
    return reply == OK;
}


bool Put(int connfd, Key key, Value value) {
    char putCommand[265] = {PUT};
    memcpy(putCommand + 1, &key, 8);
    memcpy(putCommand + 9, value.bytes, 256);
    send_n(connfd, putCommand, sizeof putCommand);
    char reply;
    recv_n(connfd, &reply, 1);
    return reply == OK;
}

int main(int argc, char *argv[]) {
    int connfd = connect_to(argv[1], argv[2]);
    for (int i = 1; i < 100; ++i) {
        std::string valueStr = std::to_string(i * 10);
        Byte value[256] = {0};
        memcpy(value, valueStr.c_str(), valueStr.size() + 1);
        Put(connfd, i, value);
    }
    for (int i = 1; i < 100; ++i) {
        std::cout << Get(connfd, i).bytes << std::endl;
    }
    close(connfd);
}