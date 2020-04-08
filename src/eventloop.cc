#include <eventloop.h>
#include <poller.h>
#include <channel.h>
#include <vector>

EventLoop::EventLoop() : poller(new Poller(this)) {
    
}

void EventLoop::updateChannel(Channel* channel) {
    poller->updateChannel(channel);
}

void EventLoop::removeChannel(Channel* channel) {
    poller->removeChannel(channel);
}

void EventLoop::loop() {
    while (true) {
        activeChannels.clear();
        poller->poll(10000, activeChannels);
        handlingEvent = true;
        for (auto channel : activeChannels) {
            currentActiveChannel = channel;
            currentActiveChannel->handleEvent();
        }
        handlingEvent = false;
        currentActiveChannel = nullptr;
    }
}

void EventLoop::runInLoop(std::function<void()> cb) {
    cb();
}