#include "server.h"

void TCPServer::create_socket() {
  listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
  if (listen_fd_ == -1) {
    throw std::system_error(errno, std::system_category(),
                            "Failed to create socket");
  }

  int enable = 1;
  if (setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &enable,
                 sizeof(enable)) == -1) {
    throw std::system_error(errno, std::system_category(),
                            "Failed to set socket options");
  }

  sockaddr_in addr{};
  std::memset(&addr, 0, sizeof(addr));
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_ANY);
  addr.sin_port = htons(port_);

  if (bind(listen_fd_, reinterpret_cast<struct sockaddr*>(&addr),
           sizeof(addr)) == -1) {
    throw std::system_error(errno, std::system_category(),
                            "Failed to bind socket");
  }

  set_non_blocking(listen_fd_);

  if (listen(listen_fd_, SOMAXCONN) == -1) {
    throw std::system_error(errno, std::system_category(),
                            "Failed to listen on socket");
  }
}

void TCPServer::set_non_blocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags == -1) {
    throw std::system_error(errno, std::system_category(),
                            "Failed to get file descriptor flags");
  }
  flags |= O_NONBLOCK;
  if (fcntl(fd, F_SETFL, flags) == -1) {
    throw std::system_error(errno, std::system_category(),
                            "Failed to set file descriptor flags");
  }
}

void TCPServer::configure_epoll() {
  epoll_fd_ = epoll_create1(0);
  if (epoll_fd_ == -1) {
    throw std::system_error(errno, std::system_category(),
                            "Failed to create epoll file descriptor");
  }

  struct epoll_event event {};
  event.data.fd = listen_fd_;
  event.events = EPOLLIN | EPOLLET;
  if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, listen_fd_, &event) == -1) {
    throw std::system_error(errno, std::system_category(),
                            "Failed to add listen socket to epoll");
  }
}

void TCPServer::handle_new_connection() {
  while (true) {
    sockaddr in_addr{};
    socklen_t in_addr_len = sizeof(in_addr);
    int client_fd = accept(listen_fd_, &in_addr, &in_addr_len);
    if (client_fd == -1) {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        throw std::system_error(errno, std::system_category(),
                                "Failed to accept new connection");
      }
      break;
    }

    std::cout << "Accepted new connection on fd " << client_fd << std::endl;
    set_non_blocking(client_fd);

    struct epoll_event event {};
    event.data.fd = client_fd;
    event.events = EPOLLIN | EPOLLET;
    if (epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, client_fd, &event) == -1) {
      throw std::system_error(errno, std::system_category(),
                              "Failed to add client socket to epoll");
    }
    client_fds_.insert(client_fd);
  }
}

void TCPServer::read_data(int fd) {
  std::string buf(1024, '\0');
  while (true) {
    ssize_t count = read(fd, buf.data(), buf.size());
    if (count == -1) {
      if (errno != EAGAIN && errno != EWOULDBLOCK) {
        throw std::system_error(errno, std::system_category(),
                                "Failed to read data from socket");
      }
      break;
    } else if (count == 0) {
      std::cout << "Connection closed by client " << fd << std::endl;
      client_fds_.erase(fd);
      close(fd);
      break;
    }
    std::cout.write(buf.data(), count);
    forward_message(fd, buf, count);
  }
}

void TCPServer::start() {
  create_socket();
  configure_epoll();
}

void TCPServer::run() {
  try {
    while (true) {
      int n = epoll_wait(epoll_fd_, events_.data(),
                         static_cast<int>(events_.size()), -1);
      if (n == -1) {
        throw std::system_error(errno, std::system_category(),
                                "Epoll wait failed");
      }

      for (int i = 0; i < n; i++) {
        if (events_[i].data.fd == listen_fd_) {
          handle_new_connection();
        } else {
          read_data(events_[i].data.fd);
        }
      }
    }
  } catch (const std::exception& e) {
    std::cerr << "Exception occurred: " << e.what() << std::endl;
    stop();
  }
}

void TCPServer::stop() {
  if (listen_fd_ != -1) {
    close(listen_fd_);
    listen_fd_ = -1;
  }
  if (epoll_fd_ != -1) {
    close(epoll_fd_);
    epoll_fd_ = -1;
  }
}

void TCPServer::forward_message(int sender_fd, const std::string& message,
                                size_t length) {
  if (!process_command(sender_fd, message)) return;
  for (int fd : client_fds_) {
    if (fd != sender_fd) {
      send(fd, message.data(), length, 0);
    }
  }
}

int TCPServer::process_command(int fd, const std::string& message) {
  if (message.substr(0, 6) == "stats\n") {
    std::string response =
        "Current connections: " + std::to_string(client_fds_.size()) + "\n";
    send(fd, response.data(), response.size(), 0);
    return 0;
  } else if (message.substr(0, 7) == "Message") {
    send_letter_count(fd, message.substr(7));
    return 0;
  }
  return 404;
}

void TCPServer::send_letter_count(int fd, const std::string& text) {
  std::map<char, int> letter_count;
  for (char c : text) {
    if (isalpha(c)) {
      letter_count[tolower(c)]++;
    }
  }

  std::ostringstream stream;
  stream << "+--------+--------+\n| Letter | Count  |\n+--------+--------+\n";
  for (const auto& pair : letter_count) {
    stream << "| " << std::left << std::setw(6) << pair.first << " | "
           << std::left << std::setw(6) << pair.second << " |\n";
  }
  stream << "+--------+--------+\n";

  send(fd, stream.str().c_str(), stream.str().size(), 0);
}
