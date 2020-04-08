#include <networking.h>
#include <noncopyable.h>

class Socket : public noncopyable {
   private:
    int fd;

   public:
    int accept(struct sockaddr_in6 *clientAddr);
    int Socket::accept(struct sockaddr_in6 *clientAddr);
    void Socket::bindAddress(const struct sockaddr* addr);
    void Socket::listen();
    int getFd() const { return fd; }
    explicit Socket(int fd) : fd(fd) {}
};