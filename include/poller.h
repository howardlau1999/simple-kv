#ifndef _POLLER_H_
#define _POLLER_H_
#include <channel.h>
#include <networking.h>
#include <eventloop.h>
#include <vector>
#include <unordered_map>
class Poller {
   private:
    int pollfd;
    EventLoop* loop;
    std::unordered_map<int, Channel*> channels;
   public:
    static const int kAdded, kNew, kDeleted;
    Poller(EventLoop* loop);
    ~Poller();
    void poll(int timeoutMs, std::vector<Channel*>& activeChannels);
    std::vector<struct epoll_event> events;
    void update(int operation, Channel* channel);
    void updateChannel(Channel* channel);
    void removeChannel(Channel* channel);
    void fillActiveChannels(int numEvents, std::vector<Channel*>& activeChannels);
};

#endif