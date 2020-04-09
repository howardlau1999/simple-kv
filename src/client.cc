#include <networking.h>
#include <command.h>

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

int main(int argc, char* argv[]) {
    int connfd = connect_to(argv[1], argv[2]);
    char putTest[265] = {1, 1, 0, 0, 0, 0, 0, 0, 0, 't', 'e', 's', 't'};
    send(connfd, putTest, 265, 0);
    char replyOK[1] = {0};
    recv_n(connfd, replyOK, 1);
    printf("%s\n", replyOK[0] == OK ? "PUT OK" : "PUT ERROR");
    char getTest[9] = {0, 1, 0, 0, 0, 0, 0, 0, 0};
    send(connfd, getTest, 9, 0);
    char replyValue[257] = {0};
    recv_n(connfd, replyValue, 257);
    printf("%s", replyValue[0] == VALUE ? "GET VALUE: " : "GET ERROR!");
    printf("%s", replyValue + 1);
    close(connfd); 
}