#include <poller.h>
#include <iostream>

const int Poller::kAdded = 1;
const int Poller::kDeleted = 2;
const int Poller::kNew = -1;

Poller::Poller(EventLoop* loop) : loop(loop), pollfd(epoll_create1(EPOLL_CLOEXEC)), events(16) {
    
}

void Poller::poll(int timeoutMs, std::vector<Channel*>& activeChannels) {
    int numEvents = ::epoll_wait(pollfd, &*events.begin(), events.size(), timeoutMs);
    if (numEvents > 0) {
        fillActiveChannels(numEvents, activeChannels);
        if (numEvents == events.size()) {
            events.resize(events.size() * 2);
        }
    }
}

void Poller::fillActiveChannels(int numEvents, std::vector<Channel*>& activeChannels) {
    for (int i = 0; i < numEvents; ++i) {
        Channel* channel = static_cast<Channel*>(events[i].data.ptr);
        channel->setReceivedEvents(events[i].events);
        activeChannels.push_back(channel);
    }
}

void Poller::update(int operation, Channel* channel) {
    struct epoll_event event;
    bzero(&event, sizeof(event));
    event.data.ptr = channel;
    event.events = channel->getEvents();
    int ret = 0;
    if (ret = ::epoll_ctl(pollfd, operation, channel->getFd(), &event)) {
        std::cerr << "epoll_ctl error: " << channel->getFd() << " " << ret << std::endl;
    }
}

void Poller::updateChannel(Channel* channel) {
    int index = channel->getIndex();
    if (index == kNew || index == kDeleted) {
        int fd = channel->getFd();
        if (index == kNew) {
            channels[fd] = channel;
        }
        channel->setIndex(kAdded);
        update(EPOLL_CTL_ADD, channel);
    } else {
        int fd = channel->getFd();
        if (channel->isNoneEvents()) {
            update(EPOLL_CTL_DEL, channel);
            channel->setIndex(kDeleted);
        } else {
            update(EPOLL_CTL_MOD, channel);
        }
    }
}

void Poller::removeChannel(Channel* channel) {
    int fd = channel->getFd();
    int index = channel->getIndex();
    channels.erase(fd);
    if (index == kAdded) {
        update(EPOLL_CTL_DEL, channel);
    }
    channel->setIndex(kNew);
}