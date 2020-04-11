#include <btree.h>
#include <command.h>
#include <networking.h>

#include <iostream>
#include <string>
#include <functional>
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

Status Get(int connfd, Key const &key, Value &value) {
    char getCommand[1 + KEY_LEN] = {GET};
    memcpy(getCommand + 1, &key, KEY_LEN);
    send_n(connfd, getCommand, sizeof getCommand);
    char retVal;
    recv_n(connfd, &retVal, 1);
    if (retVal == VALUE) {
        recv_n(connfd, (char *)value.bytes, VALUE_LEN);
        return FOUND;
    } else {
        return FAILED;
    }
}

bool Delete(int connfd, Key key) {
    char deleteCommand[KEY_LEN + 1] = {DELETE};
    memcpy(deleteCommand + 1, &key, KEY_LEN);
    send_n(connfd, deleteCommand, sizeof deleteCommand);
    char reply;
    recv_n(connfd, &reply, 1);
    return reply == OK;
}

bool Put(int connfd, Key key, Value value) {
    char putCommand[KV_LEN + 1] = {PUT};
    memcpy(putCommand + 1, &key, KEY_LEN);
    memcpy(putCommand + 1 + KEY_LEN, value.bytes, VALUE_LEN);
    send_n(connfd, putCommand, sizeof putCommand);
    char reply;
    recv_n(connfd, &reply, 1);
    return reply == OK;
}

// Scan all the keys ranged from minKey to maxKey (both included)
void Scan(int connfd, Key minKey, Key maxKey, std::function<void(KeyValue kv)> callback) {
    char scanCommand[KEY_LEN * 2 + 1] = {SCAN};
    memcpy(scanCommand + 1, &minKey, KEY_LEN);
    memcpy(scanCommand + 1 + KEY_LEN, &maxKey, KEY_LEN);
    send_n(connfd, scanCommand, sizeof scanCommand);
    while (1) {
        char reply;
        recv_n(connfd, &reply, 1);
        if (reply == NO_MORE || reply == ERROR) {
            break;
        } else {
            KeyValue kv;
            recv_n(connfd, (char*)&(kv.key), KEY_LEN);
            recv_n(connfd, (char*)kv.value.bytes, VALUE_LEN);
            callback(kv);
        }
    }
}

int main(int argc, char *argv[]) {
    int connfd = connect_to(argv[1], argv[2]);
    for (int i = 1; i < 100; ++i) {
        std::string valueStr = std::to_string(i * 10);
        Byte value[256] = {0};
        memcpy(value, valueStr.c_str(), valueStr.size() + 1);
        Put(connfd, i, value);
    }
    Delete(connfd, 4);
    Delete(connfd, 15);
    Delete(connfd, 14);
    Delete(connfd, 3);
    Byte value[256] = {'m', 'o', 'd', 'i', 'f', 'i', 'e' ,'d', '\0'};
    Put(connfd, 99, value);
    for (int i = 1; i < 100; ++i) {
        Value value;
        if (Get(connfd, i, value) == FOUND)
            std::cout << value.bytes << std::endl;
        else
            std::cout << "Key " << i << " not found!" << std::endl;
    }
    Scan(connfd, 3, 10, [](KeyValue kv) {
        std::cout << "Scanning key " << kv.key << " value " << kv.value.bytes << std::endl;
    });
    close(connfd);
}