#include "ClientCLI.h"

#include <iostream>
#include <sstream>


ClientCLI::ClientCLI() = default;


void ClientCLI::printMenu() {
    std::cout << "\n===== Available Commands ===============================\n"
            << "1. LIST               - List available files on the server\n"
            << "2. GET <filename>     - Download a file from the server\n"
            << "3. PUT <filename>     - Upload a file to the server\n"
            << "4. INFO <filename>    - Get file info from the server\n"
            << "5. DELETE <filename>  - Delete a file on the server\n"
            << "6. EXIT               - Disconnect and exit\n"
            << "=========================================================\n";
}


std::vector<std::string> ClientCLI::parseInput(const std::string &input) {
    std::istringstream iss(input);
    std::vector<std::string> tokens;
    std::string token;
    while (iss >> token) {
        tokens.push_back(token);
    }
    return tokens;
}


void ClientCLI::run(const char *serverIp, const int port) {
    client.connect(serverIp, port);

    std::string userInput;
    printMenu();

    while (true) {
        std::cout << "\nEnter command: ";
        std::getline(std::cin, userInput);
        std::vector<std::string> commandParts = parseInput(userInput);

        if (commandParts.empty()) {
            std::cout << "Invalid command. Type 'EXIT' to quit.";
            continue;
        }

        const std::string &command = commandParts[0];

        if (command == "LIST") {
            client.listFiles();
        } else if (command == "GET" && commandParts.size() == 2) {
            client.getFile(commandParts[1]);
        } else if (command == "PUT" && commandParts.size() == 2) {
            client.putFile(commandParts[1]);
        } else if (command == "INFO" && commandParts.size() == 2) {
            client.getFileInfo(commandParts[1]);
        } else if (command == "DELETE" && commandParts.size() == 2) {
            client.deleteFile(commandParts[1]);
        } else if (command == "EXIT") {
            client.disconnect();
            break;
        } else {
            std::cout <<
                    "Invalid command. Type 'LIST', 'GET <filename>', 'PUT <filename>', 'INFO <filename>', 'DELETE <filename>', or 'EXIT'.";
        }
    }
}
