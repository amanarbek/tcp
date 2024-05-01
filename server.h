#ifndef SERVER_H
#define SERVER_H

extern "C" {
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
};

#include <cstring>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

class TCPServer {
 public:
  TCPServer(int port, int max_events = 64)
      : port_(port), max_events_(max_events), epoll_fd_(-1), listen_fd_(-1) {
    events_.resize(max_events_);
  }

  ~TCPServer() { stop(); }

  void start();
  void run();
  void stop();

 private:
  int port_;
  int max_events_;
  int listen_fd_;
  int epoll_fd_;
  std::vector<struct epoll_event> events_;
  std::set<int> client_fds_;

  void create_socket();
  void set_non_blocking(int fd);
  void configure_epoll();
  void handle_new_connection();
  void read_data(int fd);
  void forward_message(int sender_fd, const std::string& message,
                       size_t length);
  int process_command(int fd, const std::string& message);

  void send_letter_count(int fd, const std::string& text);
};

#endif