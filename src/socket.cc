#include <socket.h>

Socket::~Socket() { sockets::close(fd); }

void Socket::bindAddress(const struct sockaddr* addr)
{
  sockets::bindOrDie(fd, addr);
}

void Socket::listen()
{
  sockets::listenOrDie(fd);
}

int Socket::accept(struct sockaddr_in6 *clientAddr)
{
  bzero(clientAddr, sizeof *clientAddr);
  int connfd = sockets::accept(fd, clientAddr);
  return connfd;
}
