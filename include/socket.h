#ifndef _SOCKET_H_
#define _SOCKET_H_
#include <networking.h>
#include <noncopyable.h>

class Socket : public noncopyable {
   private:
    int fd;

   public:
    int accept(struct sockaddr_in6 *clientAddr);
    void bindAddress(const struct sockaddr* addr);
    void listen();
    int getFd() const { return fd; }
    explicit Socket(int fd) : fd(fd) {}
    ~Socket();
};
#endif