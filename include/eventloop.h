#ifndef _EVENTLOOP_H_
#define _EVENTLOOP_H_
#include <memory>
#include <functional>
#include <vector>
class Channel;
class Poller;
class EventLoop {
public:
    EventLoop();
    ~EventLoop();

    void loop();
    void quit();
    void runInLoop(std::function<void()> cb);
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
private:
    std::vector<Channel*> activeChannels;
    std::unique_ptr<Poller> poller;
    Channel* currentActiveChannel;
    bool handlingEvent;
};
#endif