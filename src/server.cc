#include <acceptor.h>
#include <btree.h>
#include <networking.h>

int get_in_port(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return (((struct sockaddr_in *)sa)->sin_port);
    }

    return (((struct sockaddr_in6 *)sa)->sin6_port);
}

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

int main(int argc, char *argv[]) {
    BTree *tree = new BTree(1024);
    struct addrinfo hints, *servinfo, *p;
    EventLoop *loop = new EventLoop();
    Acceptor *acceptor;
    const int BACKLOG = 10;
    int rv, fd;
    int yes = 1;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    if ((rv = getaddrinfo("127.0.0.1", argv[1], &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    for (p = servinfo; p != NULL; p = p->ai_next) {
        acceptor = new Acceptor(loop, p);
        break;
    }

    freeaddrinfo(servinfo);
    acceptor->listen();
    acceptor->setNewConnectionCallback(
        [](int connfd, struct sockaddr_storage clientAddr) {
            char s[INET6_ADDRSTRLEN] = {0};
            inet_ntop(clientAddr.ss_family,
                      get_in_addr((struct sockaddr *)&clientAddr), s, sizeof s);
            printf("server: accepted connection from %s:%d\n", s,
                   get_in_port((struct sockaddr *)&clientAddr));
            send(connfd, "hello!\n", 7, 0);
        });
    loop->loop();
}