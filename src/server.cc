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
                case SCAN:
                    if (inputBuffer.size() >= 2 * KEY_LEN) {
                        Key key[2];
                        std::copy(inputBuffer.begin(),
                                  inputBuffer.begin() + 2 * KEY_LEN,
                                  (char *)&key);
                        for (Iterator it = tree->seek(key[0]);
                             it.isValid() && it.getKV().key <= key[1];
                             it.next()) {
                            int ret = Operation::KV;
                            conn->send(&ret, 1);
                            KeyValue kv = it.getKV();
                            conn->send(&(kv.key), KEY_LEN);
                            conn->send(kv.value.bytes, VALUE_LEN);
                        }
                        int ret = Operation::NO_MORE;
                        conn->send(&ret, 1);
                        processedBytes = 2 * KEY_LEN;
                        conn->isProcessing = false;
                    }
                    break;
                case GET:
                    if (inputBuffer.size() >= KEY_LEN) {
                        Key key;
                        Value value;
                        std::copy(inputBuffer.begin(), inputBuffer.begin() + 8,
                                  (char *)&key);
                        Status status = tree->find(key, value);
                        if (status == FAILED) {
                            int ret = Operation::NOT_FOUND;
                            conn->send(&ret, 1);
                        } else {
                            int ret = Operation::VALUE;
                            conn->send(&ret, 1);
                            conn->send(value.bytes, VALUE_LEN);
                        }
                        processedBytes = KEY_LEN;
                        conn->isProcessing = false;
                    }
                    break;
                case DELETE:
                    if (inputBuffer.size() >= KEY_LEN) {
                        Key key;
                        std::copy(inputBuffer.begin(), inputBuffer.begin() + 8,
                                  (char *)&key);
                        tree->remove(key);
                        int ok = Operation::OK;
                        conn->send(&ok, 1);
                        conn->isProcessing = false;
                        processedBytes = KEY_LEN;
                    }
                    break;
                case PUT:
                    if (inputBuffer.size() >= KV_LEN) {
                        Key key;
                        Value value;
                        std::copy(inputBuffer.begin(),
                                  inputBuffer.begin() + KEY_LEN,
                                  (char *)&(key));
                        std::copy(inputBuffer.begin() + KEY_LEN,
                                  inputBuffer.begin() + KV_LEN, value.bytes);
                        tree->insert(key, value);
                        int ok = Operation::OK;
                        conn->send(&ok, 1);
                        conn->isProcessing = false;
                        processedBytes = KV_LEN;
                    }
                    break;
                default:
                    std::cerr << "UNKNOWN COMMAND " << conn->currentCommand.op
                              << std::endl;
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