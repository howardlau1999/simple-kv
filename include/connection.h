#ifndef _CONNECTION_H_
#define _CONNECTION_H_

#include <btree.h>
#include <channel.h>
#include <command.h>
#include <eventloop.h>
#include <socket.h>

#include <deque>
class Connection : public std::enable_shared_from_this<Connection> {
   public:
    Connection(EventLoop* loop, BTree* tree, int sockfd, int connid);
    int getFd() { return sockfd; };
    EventLoop* getLoop() { return loop; }
    typedef std::function<void(std::shared_ptr<Connection> conn)>
        MessageCallback;
    typedef std::function<void(std::shared_ptr<Connection> conn)> CloseCallback;
    typedef std::function<void(std::shared_ptr<Connection> conn)>
        WriteCompleteCallback;
    int getConnId() { return connid; }
    void send(const void* buffer, const int len);
    void handleRead();
    void handleWrite();
    void handleClose();
    void setMessageCallback(MessageCallback cb) { messageCallback = cb; }
    void setCloseCallback(CloseCallback cb) { closeCallback = cb; }
    void connectionEstablished() { this->channel->enableReading(); }

    std::vector<char>& getInputBuffer() { return this->inputBuffer; }

    void connectionDestroyed() {
        this->channel->disableAll();
        this->channel->remove();
    }
    bool isProcessing;
    Command currentCommand;

   private:
    int sockfd;
    int connid;
    EventLoop* loop;
    std::vector<char> inputBuffer;
    std::vector<char> outputBuffer;
    std::deque<Command> commands;
    std::unique_ptr<Socket> socket;
    std::unique_ptr<Channel> channel;
    MessageCallback messageCallback;
    CloseCallback closeCallback;
};
#endif