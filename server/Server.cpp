#include "Server.h"

#include <iostream>
#include <thread>
#include <netinet/in.h>
#include <unistd.h>


void Server::start(const int port) {
    if (!serverSocket.createS()) {
        return;
    }

    // bind and listen
    if (!serverSocket.bindS(port) || !serverSocket.listenS(1)) {
        serverSocket.closeS();
        return;
    }

    std::cout << "Server listening on port " << port << std::endl;

    run();
}


void Server::stop() const {
    serverSocket.closeS();
    std::cout << "Server stopped." << std::endl;
}


void Server::run() const {
    while (true) {
        Socket clientSocket = acceptClient();
        if (clientSocket.getS() != -1) {
            handleClient(clientSocket);
            clientSocket.closeS();
            std::cout << "Client disconnected." << std::endl;
        } else {
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
}


Socket Server::acceptClient() const {
    sockaddr_in clientAddr{};
    socklen_t clientAddrLen = sizeof(clientAddr);

    std::cout << "Waiting for a client to connect..." << std::endl;
    int clientfd = serverSocket.acceptS(&clientAddr, &clientAddrLen);

    Socket clientSocket;
    clientSocket.setS(clientfd);
    std::cout << "Client connected!" << std::endl;
    return clientSocket;
}


void Server::handleClient(const Socket clientSocket) {
    char buffer[1024];
    while (true) {
        memset(buffer, 0, 1024);
        const ssize_t bytesReceived = clientSocket.receiveData(buffer, sizeof(buffer) - 1);
        std::cout << bytesReceived << std::endl;

        if (bytesReceived <= 0) {
            break;
        }

        std::string command(buffer);
        std::cout << "Received data: " << buffer << std::endl;

        ssize_t bytesSent;
        if (command == "EXIT") {
            bytesSent = clientSocket.sendData("Goodbye!");
            std::cout << bytesSent << std::endl;
            break;
        }
        bytesSent = clientSocket.sendData("Command received.");
        std::cout << bytesSent << std::endl;
    }
}
