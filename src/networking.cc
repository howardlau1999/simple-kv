#include <networking.h>
#include <iostream>

int get_in_port(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return (((struct sockaddr_in *)sa)->sin_port);
    }

    return (((struct sockaddr_in6 *)sa)->sin6_port);
}

void *get_in_addr(struct sockaddr *sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in *)sa)->sin_addr);
    }

    return &(((struct sockaddr_in6 *)sa)->sin6_addr);
}

const struct sockaddr* sockets::sockaddr_cast(const struct sockaddr_in6* addr) {
    return static_cast<const struct sockaddr*>(
        reinterpret_cast<const void*>(addr));
}

struct sockaddr* sockets::sockaddr_cast(struct sockaddr_in6* addr) {
    return static_cast<struct sockaddr*>(reinterpret_cast<void*>(addr));
}

const struct sockaddr* sockets::sockaddr_cast(const struct sockaddr_in* addr) {
    return static_cast<const struct sockaddr*>(
        reinterpret_cast<const void*>(addr));
}

const struct sockaddr_in* sockets::sockaddr_in_cast(
    const struct sockaddr* addr) {
    return static_cast<const struct sockaddr_in*>(
        reinterpret_cast<const void*>(addr));
}

const struct sockaddr_in6* sockets::sockaddr_in6_cast(
    const struct sockaddr* addr) {
    return static_cast<const struct sockaddr_in6*>(
        reinterpret_cast<const void*>(addr));
}

int sockets::createNonblockingOrDie(sa_family_t family) {
    int sockfd = ::socket(family, SOCK_STREAM | SOCK_NONBLOCK | SOCK_CLOEXEC,
                          IPPROTO_TCP);
    if (sockfd < 0) {
        std::cerr << "sockets::createNonblockingOrDie";
    }
    return sockfd;
}

void sockets::bindOrDie(int sockfd, const struct sockaddr* addr) {
    int ret = ::bind(sockfd, addr,
                     static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
    if (ret < 0) {
        std::cerr << "sockets::bindOrDie";
    }
}

void sockets::listenOrDie(int sockfd) {
    int ret = ::listen(sockfd, SOMAXCONN);
    if (ret < 0) {
        std::cerr << "sockets::listenOrDie";
    }
}

int sockets::accept(int sockfd, struct sockaddr_in6* addr) {
    socklen_t addrlen = static_cast<socklen_t>(sizeof *addr);
    int connfd = ::accept4(sockfd, sockaddr_cast(addr), &addrlen,
                           SOCK_NONBLOCK | SOCK_CLOEXEC);
    if (connfd < 0) {
        int savedErrno = errno;
        std::cerr << "sockets::accept";
        switch (savedErrno) {
            case EAGAIN:
            case ECONNABORTED:
            case EINTR:
            case EPROTO:  // ???
            case EPERM:
            case EMFILE:  // per-process lmit of open file desctiptor ???
                // expected errors
                errno = savedErrno;
                break;
            case EBADF:
            case EFAULT:
            case EINVAL:
            case ENFILE:
            case ENOBUFS:
            case ENOMEM:
            case ENOTSOCK:
            case EOPNOTSUPP:
                // unexpected errors
                std::cerr << "unexpected error of ::accept " << savedErrno;
                break;
            default:
                std::cerr << "unknown error of ::accept " << savedErrno;
                break;
        }
    }
    return connfd;
}

int sockets::connectTo(int sockfd, const struct sockaddr* addr) {
    return ::connect(sockfd, addr,
                     static_cast<socklen_t>(sizeof(struct sockaddr_in6)));
}

ssize_t sockets::read(int sockfd, void* buf, size_t count) {
    return ::read(sockfd, buf, count);
}

ssize_t sockets::readv(int sockfd, const struct iovec* iov, int iovcnt) {
    return ::readv(sockfd, iov, iovcnt);
}

ssize_t sockets::write(int sockfd, const void* buf, size_t count) {
    return ::write(sockfd, buf, count);
}

void sockets::close(int sockfd) {
    if (::close(sockfd) < 0) {
        std::cerr << "sockets::close";
    }
}

void sockets::shutdownWrite(int sockfd) {
    if (::shutdown(sockfd, SHUT_WR) < 0) {
        std::cerr << "sockets::shutdownWrite";
    }
}