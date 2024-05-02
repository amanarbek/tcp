#ifndef CLIENT_H
#define CLIENT_H

extern "C" {
#include <arpa/inet.h>
#include <fcntl.h>
#include <poll.h>
#include <sys/socket.h>
#include <unistd.h>
}

#include <atomic>
#include <cstring>
#include <iostream>
#include <thread>

static std::atomic<bool> isConnected(false);

class Client {
 private:
  int sock_;
  std::string address_;
  int port_;
  struct sockaddr_in server_;

 public:
  Client(const std::string &addr, int port)
      : address_(addr), port_(port), sock_(-1) {}

  ~Client() {
    if (sock_ != -1) close(sock_);
  }

  void connectToServer();
  void receiveMessages();
  void sendMessages();
};

#endif