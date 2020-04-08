#include <channel.h>

void Channel::handleEvent() {
    handlingEvent = true;
    if ((revents & EPOLLHUP) && !(revents & EPOLLIN)) {
        if (closeCallback) closeCallback();
    }

    if (revents & (EPOLLERR)) {
        if (errorCallback) errorCallback();
    }
    if (revents & (EPOLLIN | EPOLLPRI | EPOLLRDHUP)) {
        if (readCallback) readCallback();
    }
    if (revents & EPOLLOUT) {
        if (writeCallback) writeCallback();
    }
    handlingEvent = false;
}