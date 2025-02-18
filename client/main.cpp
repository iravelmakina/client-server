#include "Client.h"
#include <iostream>

int main() {
    Client client;
    client.connect("127.0.0.1", 8080);

    std::string command;
    while (true) {
        std::cout << "\nEnter a command: ";
        std::getline(std::cin, command);

        if (command == "exit") {
            client.disconnect();
            break;
        }

        client.sendCommand(command.c_str());
        client.receiveResponse();
    }
    return 0;
}
