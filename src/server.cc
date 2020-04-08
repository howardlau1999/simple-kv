#include <acceptor.h>
#include <btree.h>
#include <connection.h>
#include <networking.h>
#include <unordered_map>
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
    std::unordered_map<int, std::shared_ptr<Connection>> connections;
    int connid = 1;
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
        [loop, tree, &connid, &connections](int connfd, struct sockaddr_storage clientAddr) {
            char s[INET6_ADDRSTRLEN] = {0};
            char str[] = "Hello!\n";
            inet_ntop(clientAddr.ss_family,
                      get_in_addr((struct sockaddr *)&clientAddr), s, sizeof s);
            printf("server: accepted connection %d from %s:%d\n", connid, s,
                   get_in_port((struct sockaddr *)&clientAddr));
            std::shared_ptr<Connection> conn(new Connection(loop, tree, connfd, connid));
            connections[connid++] = conn;
            conn->connectionEstablished();
            conn->send(str, sizeof(str));
            conn->setMessageCallback([](std::shared_ptr<Connection> conn) {
                auto& buffer = conn->getInputBuffer();
                std::string str(buffer.begin(), buffer.end());
                buffer.clear();
                std::cout << "server: conn " << conn->getConnId() << " : " << str;
            });
            conn->setCloseCallback([loop, &connections](std::shared_ptr<Connection> conn) {
                connections.erase(conn->getConnId());
                loop->runInLoop(std::bind(&Connection::connectionDestroyed, conn));
                std::cout << "server: connection id " << conn->getConnId() << " closed" << std::endl;
            });
        });
    loop->loop();
}