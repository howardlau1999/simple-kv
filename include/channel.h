#ifndef _CHANNEL_H_
#define _CHANNEL_H_

#include <eventloop.h>
#include <networking.h>
#include <functional>
class EventLoop;
class Channel {
   public:
    typedef std::function<void()> CallbackFn;
    Channel(EventLoop* loop, int fd) : loop(loop), fd(fd), index(-1) {}
    ~Channel() = default;

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
    void enableReading() { this->events |= (EPOLLIN | EPOLLRDHUP); update(); }
    void disableReading() { this->events &= ~(EPOLLIN | EPOLLRDHUP); update(); }
    void enableWriting() { this->events |= (EPOLLOUT); update(); }
    void disableWriting() { this->events &= ~(EPOLLOUT); update(); }
    void disableAll() { this->events = 0; update(); }
    bool isWriting() { return (this->events & EPOLLOUT) == EPOLLOUT; }
    bool isReading() { return (this->events & EPOLLIN) == EPOLLIN; }
    bool isNoneEvents() { return this->events == 0; }
    void update() { loop->updateChannel(this); }
    void remove() { loop->removeChannel(this); }
   private:
    EventLoop* loop;
    int fd;
    int index;
    int events;
    int revents;
    bool handlingEvent;
    CallbackFn readCallback, writeCallback, closeCallback, errorCallback;
};

#endif