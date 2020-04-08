#include <connection.h>

Connection::Connection(EventLoop* loop, BTree* btree, int sockfd, int connid)
    : sockfd(sockfd),
      socket(new Socket(sockfd)),
      channel(new Channel(loop, sockfd)),
      connid(connid),
      loop(loop) {
    channel->setReadCallback(std::bind(&Connection::handleRead, this));
    channel->setWriteCallback(std::bind(&Connection::handleWrite, this));
    channel->setCloseCallback(std::bind(&Connection::handleClose, this));
};

void Connection::send(const void* buf, int len) {
    const char* buffer = reinterpret_cast<const char*>(buf);
    int n = 0;
    int remaining = len;
    if (!channel->isWriting() && outputBuffer.empty()) {
        n = sockets::write(channel->getFd(), buffer, len);
        if (n >= 0) {
            remaining = len - n;
            if (remaining != 0) {
                channel->enableWriting();
                this->outputBuffer.insert(this->outputBuffer.end(), buffer + n,
                                          buffer + len);
            }
        }
    }
    if (n >= 0) {
        this->outputBuffer.insert(this->outputBuffer.end(), buffer + n,
                                  buffer + len);

        if (!channel->isWriting()) {
            channel->enableWriting();
        }
    }
}

void Connection::handleClose() {
    channel->disableAll();
    if (closeCallback) {
        closeCallback(shared_from_this());
    }
}

void Connection::handleWrite() {
    if (channel->isWriting()) {
        int n = sockets::write(channel->getFd(), &*outputBuffer.begin(),
                               outputBuffer.size());
        if (n > 0) {
            if (n >= outputBuffer.size()) {
                outputBuffer.clear();
                channel->disableWriting();
            } else {
                std::vector<char>(outputBuffer.begin() + n, outputBuffer.end())
                    .swap(outputBuffer);
            }
        }
    }
}

void Connection::handleRead() {
    char buffer[BUFFER_LEN];
    int n = sockets::read(channel->getFd(), buffer, BUFFER_LEN);
    if (n > 0) {
        this->inputBuffer.insert(this->inputBuffer.end(), buffer, buffer + n);
        if (messageCallback) {
            messageCallback(shared_from_this());
        }
    } else {
        handleClose();
    }
}