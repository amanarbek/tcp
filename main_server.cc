#include "server.h"

int main() {
  try {
    TCPServer server(9000);
    server.start();
    server.run();
  } catch (const std::exception& e) {
    std::cerr << "Failed to start server: " << e.what() << std::endl;
    return 1;
  }
  return 0;
}
