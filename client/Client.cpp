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
    clientSocket.sendData("EXIT");
    std::cout << "Disconnected from server." << std::endl;
}


void Client::sendCommand(const char* command) const {
    clientSocket.sendData(command);
}


std::string Client::receiveResponse() const {
    char buffer[512] = {};
    const ssize_t bytesReceived = clientSocket.receiveData(buffer, sizeof(buffer) - 1);
    if (bytesReceived > 0) {
        std::cout << "Server Response:\n" << buffer << std::endl;
        return {buffer};
    }
    std::cout << "No response from server." << std::endl;
    return "";
}


void Client::downloadFile(const std::string &filename) const {
    const std::string response = receiveResponse();
    if (response.find("200 OK") != 0) {
        std::cout << "Error: " << response << std::endl;
        return;
    }
    clientSocket.sendData("ACK");

    uint32_t fileSize;
    clientSocket.receiveData(fileSize);
    clientSocket.sendData("ACK");

    const int fileFd = open(("../../client/files/" + filename).c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fileFd == -1) {
        std::cout << "Error: Cannot create file\n";
        return;
    }

    char buffer[1024];
    ssize_t bytesReceived;
    size_t totalReceived = 0;

    while (totalReceived < fileSize && (bytesReceived = clientSocket.receiveData(buffer, sizeof(buffer))) > 0) {
        write(fileFd, buffer, bytesReceived);
        totalReceived += bytesReceived;
    }
    close(fileFd);
}


void Client::uploadFile(const std::string &filename) const {
    std::string response = receiveResponse();
    if (response.find("200 OK") != 0) {
        std::cout << "Error: " << response << std::endl;
        return;
    }

    const int fileFd = open(("../../client/files/" + filename).c_str(), O_RDONLY);
    if (fileFd == -1) {
        std::cout << "Error: Cannot open file\n";
        return;
    }

    struct stat fileStat{};
    fstat(fileFd, &fileStat);
    const uint32_t fileSize = fileStat.st_size;
    clientSocket.sendData(fileSize);

    response = receiveResponse();
    if (response.find("ACK") != 0) {
        std::cout << "Error: Server did not acknowledge file size.\n";
        return;
    }

    char buffer[1024];
    ssize_t bytesRead;

    while ((bytesRead = read(fileFd, buffer, sizeof(buffer))) > 0) {
        send(clientSocket.getS(), buffer, bytesRead, 0);
    }

    close(fileFd);

    response = receiveResponse();
    if (response.find("200 OK") == 0) {
        std::cout << "Upload successful.\n";
    } else {
        std::cerr << "Error: " << response << std::endl;
    }
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

