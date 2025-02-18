#include "Client.h"

#include <iostream>


void Client::connect(const char* serverIp, const int port) {
    if (!clientSocket.createS()) {
        return;
    }

    if (!clientSocket.connectS(serverIp, port)) {
        clientSocket.closeS();
        return;
    }
    std::cout << "Connected to server at " << serverIp << ":" << port << std::endl;
}


void Client::disconnect() const {
    clientSocket.closeS();
    std::cout << "Disconnected from server." << std::endl;
}


void Client::sendCommand(const char* command) const {
    const ssize_t bytesSent = clientSocket.sendData(command);
    std::cout << bytesSent << std::endl;
}


void Client::receiveResponse() const {
    char buffer[1024];
    memset(buffer, 0, 1024);
    const ssize_t bytesReceived = clientSocket.receiveData(buffer, sizeof(buffer) - 1);
    std::cout << bytesReceived << std::endl;
    if (bytesReceived > 0) {
        std::cout << "Received data: " << buffer << std::endl;
    } else {
        std::cout << "No response from server." << std::endl;
    }
}

