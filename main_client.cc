#include "client.h"

int main(int argc, char* argv[]) {
  try {
    if (argc != 3) {
      std::cerr << "Usage: " << argv[0] << " <ip> <port>\n";
      return 1;
    }

    std::string ip = argv[1];
    int port = atoi(argv[2]);

    Client client(ip, port);
    client.connectToServer();

    std::thread receiverThread(&Client::receiveMessages, &client);
    std::thread senderThread(&Client::sendMessages, &client);

    receiverThread.join();
    senderThread.join();

    std::cout << "Client is shutting down.\n";

  } catch (const std::system_error& e) {
    std::cerr << "System Error: " << e.what() << " (Code " << e.code() << ")"
              << std::endl;
  }

  return 0;
}
