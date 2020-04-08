#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include <eventloop.h>

#include <functional>

class Channel {
   public:
    typedef std::function<void()> CallbackFn;
    Channel(EventLoop* loop, int fd) : loop(loop), fd(fd) {}
    ~Channel();

    int getFd() { return fd; }

    int getEvents() { return events; }

    int getReceivedEvents() { return revents; }

    void setReceivedEvents(int revents) { this->revents = revents; }
    int getIndex() { return index; }

    void setIndex(int index) { this->index = index; }
    void handleEvent();
    void setReadCallback(CallbackFn cb) { readCallback = std::move(cb); }
    void setWriteCallback(CallbackFn cb) { writeCallback = std::move(cb); }
    void setCloseCallback(CallbackFn cb) { closeCallback = std::move(cb); }
    void setErrorCallback(CallbackFn cb) { errorCallback = std::move(cb); }
    void enableReading() { this->events |= (EPOLLIN | EPOLLRDHUP); }
    void disableReading() { this->events &= ~(EPOLLIN | EPOLLRDHUP); }
    void enableWriting() { this->events |= (EPOLLOUT); }
    void disableWriting() { this->events &= ~(EPOLLOUT); }
    void disableAll() { this->events = 0; }
    void isWriting() { return (this->events & EPOLLOUT) == EPOLLOUT; }
    void isReading() { return (this->events & EPOLLIN) == EPOLLIN; }
    bool isNoneEvents() { return this->events == 0; }
    void update() { loop->updateChannel(this); }
    void remove() { loop->removeChannel(this); }
    void handleEvent();
   private:
    EventLoop* loop;
    int fd;
    int index;
    int events;
    int revents;
    CallbackFn readCallback, writeCallback, closeCallback, errorCallback;
};

#endif