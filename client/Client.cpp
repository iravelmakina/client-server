// ReSharper disable CppExpressionWithoutSideEffects
#include "Client.h"

#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>


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
    clientSocket.sendData(command);
}


void Client::receiveResponse() const {
    char buffer[1024];
    memset(buffer, '\0', 1024);
    const ssize_t bytesReceived = clientSocket.receiveData(buffer, sizeof(buffer) - 1);
    if (bytesReceived > 0) {
        std::cout << "Received data:\n" << buffer << std::endl;
    } else {
        std::cout << "No response from server." << std::endl;
    }
}


void Client::downloadFile(const std::string &filename) const {
    uint32_t fileSize;
    clientSocket.receiveData(fileSize);
    std::cout << "File size: " << fileSize << " bytes" << std::endl;

    const int fileFd = open(("../../client/files/" + filename).c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fileFd == -1) {
        std::cerr << "Error: Cannot create file\n";
        return;
    }
    // what if already exists

    char buffer[1024];
    ssize_t bytesReceived;
    size_t totalReceived = 0;

    while (totalReceived < fileSize && (bytesReceived = clientSocket.receiveData(buffer, sizeof(buffer))) > 0) {
        write(fileFd, buffer, bytesReceived);
        totalReceived += bytesReceived;
    }
}


void Client::uploadFile(const std::string &filename) const {
    const int fileFd = open(("../../client/files/" + filename).c_str(), O_RDONLY);

    if (fileFd == -1) {
        std::cerr << "Error: Cannot open file\n";
        return;
    }

    struct stat fileStat{};
    fstat(fileFd, &fileStat);
    const uint32_t fileSize = fileStat.st_size;

    clientSocket.sendData(fileSize);
    std::cout << "File size: " << fileSize << " bytes" << std::endl;

    char buffer[1024];
    ssize_t bytesRead;

    while ((bytesRead = read(fileFd, buffer, sizeof(buffer))) > 0) {
        send(clientSocket.getS(), buffer, bytesRead, 0);
    }

    close(fileFd);
}


void Client::listFiles() const {
    sendCommand("LIST");
    receiveResponse();
}


void Client::getFile(const std::string& filename) const {
    sendCommand(("GET " + filename).c_str());
    downloadFile(filename);
}


void Client::putFile(const std::string& filename) const {
    sendCommand(("PUT " + filename).c_str());
    uploadFile(filename);
}


void Client::deleteFile(const std::string& filename) const {
    sendCommand(("DELETE " + filename).c_str());
    receiveResponse();
}


void Client::getFileInfo(const std::string& filename) const {
    sendCommand(("INFO " + filename).c_str());
    receiveResponse();
}

// overload method

