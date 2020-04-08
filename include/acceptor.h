#ifndef _ACCEPTOR_H_
#define _ACCPETOR_H_

#include <channel.h>
#include <networking.h>
#include <socket.h>
#include <iostream>

class Acceptor {
   public:
    typedef std::function<void(int sockfd, const struct sockaddr_storage)>
        NewConnectionCallback;
    Acceptor(EventLoop* loop, const struct addrinfo* addr)
        : listening(false),
          acceptSocket(sockets::createNonblockingOrDie(addr->ai_family)),
          acceptChannel(loop, acceptSocket.getFd()) {
        acceptSocket.bindAddress(addr->ai_addr);
        acceptChannel.setReadCallback(std::bind(&Acceptor::handleRead, this));
    }

    bool isListening() { return this->listening; }

    void setNewConnectionCallback(NewConnectionCallback cb) {
        this->newConnectionCallback = cb;
    }

    void handleRead() {
        struct sockaddr_storage clientAddr;
        int connfd = acceptSocket.accept(reinterpret_cast<sockaddr_in6*>(&clientAddr));
        if (connfd < 0) {
            std::cerr << "Cannot accept" << std::endl;
        }
        if (newConnectionCallback) {
            newConnectionCallback(connfd, clientAddr);
        } else {
            sockets::close(connfd);
        }
    }

    ~Acceptor() {
        acceptChannel.disableAll();
        acceptChannel.remove();
    }

    void listen() {
        listening = true;
        acceptSocket.listen();
        acceptChannel.enableReading();
    }

   private:
    Socket acceptSocket;
    Channel acceptChannel;
    bool listening;
    NewConnectionCallback newConnectionCallback;
};
#endif