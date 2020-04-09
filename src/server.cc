#include <acceptor.h>
#include <btree.h>
#include <command.h>
#include <connection.h>
#include <networking.h>

#include <unordered_map>
BTree *tree;

void onMessage(std::shared_ptr<Connection> conn) {
    auto &inputBuffer = conn->getInputBuffer();
    int processedBytes = -1;
    while (inputBuffer.size() >= 1 && processedBytes != 0) {
        processedBytes = 0;
        if (conn->isProcessing) {
            switch (conn->currentCommand.op) {
                case GET:
                    if (inputBuffer.size() >= 8) {
                        Key key;
                        std::copy(inputBuffer.begin(), inputBuffer.begin() + 8,
                                  (char *)&key);
                        Value v = tree->find(key);
                        int value = Operation::VALUE;
                        conn->send(&value, 1);
                        conn->send(v.bytes, 256);
                        conn->isProcessing = false;
                        processedBytes = 8;
                    }
                    break;
                case DELETE:
                    if (inputBuffer.size() >= 8) {
                        Key key;
                        std::copy(inputBuffer.begin(), inputBuffer.begin() + 8,
                                  (char *)&key);
                        tree->remove(key);
                        int ok = Operation::OK;
                        conn->send(&ok, 1);
                        conn->isProcessing = false;
                        processedBytes = 8;
                    }
                    break;
                case PUT:
                    if (inputBuffer.size() >= 264) {
                        Key key;
                        Value value;
                        std::copy(inputBuffer.begin(), inputBuffer.begin() + 8,
                                  (char *)&(key));
                        std::copy(inputBuffer.begin() + 8,
                                  inputBuffer.begin() + 264, value.bytes);
                        tree->insert(key, value);
                        int ok = Operation::OK;
                        conn->send(&ok, 1);
                        conn->isProcessing = false;
                        processedBytes = 264;
                    }
                    break;
                default:
                    break;
            }
        } else {
            conn->currentCommand.op = static_cast<Operation>(inputBuffer[0]);
            conn->isProcessing = true;
            processedBytes = 1;
        }
        if (processedBytes > 0)
            std::vector<char>(inputBuffer.begin() + processedBytes,
                              inputBuffer.end())
                .swap(inputBuffer);
    }
}

int main(int argc, char *argv[]) {
    tree = new BTree(1024);
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
    acceptor->setNewConnectionCallback([loop, tree, &connid, &connections](
                                           int connfd,
                                           struct sockaddr_storage clientAddr) {
        char s[INET6_ADDRSTRLEN] = {0};
        char str[] = "Hello!\n";
        inet_ntop(clientAddr.ss_family,
                  get_in_addr((struct sockaddr *)&clientAddr), s, sizeof s);
        printf("server: accepted connection %d from %s:%d\n", connid, s,
               get_in_port((struct sockaddr *)&clientAddr));
        std::shared_ptr<Connection> conn(
            new Connection(loop, tree, connfd, connid));
        connections[connid++] = conn;
        conn->connectionEstablished();
        conn->setMessageCallback(onMessage);
        conn->setCloseCallback([loop, &connections](
                                   std::shared_ptr<Connection> conn) {
            connections.erase(conn->getConnId());
            loop->runInLoop(std::bind(&Connection::connectionDestroyed, conn));
            std::cout << "server: connection id " << conn->getConnId()
                      << " closed" << std::endl;
        });
    });
    loop->loop();
    delete tree;
}