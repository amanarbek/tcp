#include "client.h"

void Client::connectToServer() {
  sock_ = socket(AF_INET, SOCK_STREAM, 0);
  if (sock_ == -1) {
    throw std::system_error(errno, std::system_category(),
                            "Could not create socket");
  }

  server_.sin_addr.s_addr = inet_addr(address_.c_str());
  server_.sin_family = AF_INET;
  server_.sin_port = htons(port_);

  if (connect(sock_, (struct sockaddr *)&server_, sizeof(server_)) < 0) {
    throw std::system_error(errno, std::system_category(), "Connect failed");
  }

  std::cout << "Connected to " << address_ << ":" << port_ << std::endl;
  isConnected = true;

  int flags = fcntl(sock_, F_GETFL, 0);
  fcntl(sock_, F_SETFL, flags | O_NONBLOCK);
}

void Client::receiveMessages() {
  char buffer[1024];
  while (isConnected) {
    memset(buffer, 0, sizeof(buffer));
    int bytesReceived = recv(sock_, buffer, sizeof(buffer), 0);
    if (bytesReceived > 0) {
      std::cout << std::string(buffer, bytesReceived);
    } else if (bytesReceived == 0) {
      std::cerr << "Server connection closed.\n";
      isConnected = false;
    } else if (errno != EWOULDBLOCK) {
      std::cerr << "Error in receiving data.\n";
      isConnected = false;
    }
  }
}

void Client::sendMessages() {
  std::string message;
  while (isConnected) {
    struct pollfd pfd = {STDIN_FILENO, POLLIN, 0};
    int ret = poll(&pfd, 1, 1000);

    if (ret > 0 && std::getline(std::cin, message)) {
      message += "\n";
      if (message == "exit\n") {
        isConnected = false;
        break;
      }
      const char *data = message.c_str();
      size_t to_send = message.size();
      ssize_t sent = 0;
      while (to_send > 0) {
        sent = send(sock_, data, to_send, 0);
        if (sent < 0) {
          if (errno != EWOULDBLOCK) {
            std::cerr << "Failed to send message: " << strerror(errno) << "\n";
            isConnected = false;
            break;
          }
        }
        to_send -= sent;
        data += sent;
      }
    } else if (ret < 0) {
      std::cerr << "Error polling stdin.\n";
      break;
    }
  }
}
