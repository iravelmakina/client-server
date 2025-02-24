// ReSharper disable CppExpressionWithoutSideEffects
#include "Client.h"

#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>


void Client::connect(const char* serverIp, const int port) {
    if (!_socket.createS()) {
        return;
    }

    if (!_socket.connectS(serverIp, port)) {
        _socket.closeS();
        return;
    }

    if (receiveResponse().find("CONNECTED_OK") != 0) {
        return;
    }

    std::cout << "\nConnected to server at " << serverIp << ":" << port << std::endl;
}


void Client::disconnect() const {
    _socket.sendData("EXIT");
    std::cout << "\nDisconnected from server." << std::endl;
}


std::string Client::receiveResponse() const {
    char buffer[512] = {};
    const ssize_t bytesReceived = _socket.receiveData(buffer, sizeof(buffer) - 1);
    if (bytesReceived > 0) {
        return {buffer};
    }
    std::cout << "No response from server." << std::endl;
    return "";
}


void Client::downloadFile(const std::string &filename) const {
    if (receiveResponse().find("200 OK") != 0) {
        return;
    }

    _socket.sendData("ACK");

    uint32_t fileSize;
    _socket.receiveData(fileSize); // error

    _socket.sendData("ACK");

    const int fileFd = open(("../../client/files/" + filename).c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fileFd == -1) {
        perror("open");
        return;
    }

    char buffer[1024];
    ssize_t bytesReceived;
    size_t totalReceived = 0;

    while (totalReceived < fileSize && (bytesReceived = _socket.receiveData(buffer, sizeof(buffer))) > 0) {
        write(fileFd, buffer, bytesReceived);
        totalReceived += bytesReceived;
    }

    close(fileFd);
    std::cout << "Download complete: " << filename << std::endl;
}


void Client::uploadFile(const std::string &filename, const int fileFd) const {
    if (receiveResponse().find("200 OK") != 0) {
        return;
    }

    struct stat fileStat{};
    fstat(fileFd, &fileStat);
    const uint32_t fileSize = fileStat.st_size;
    _socket.sendData(fileSize);

    if (receiveResponse().find("ACK") != 0) {
        std::cout << "Error: Server did not acknowledge file size.\n";
        close(fileFd);
        return;
    }

    char buffer[1024];
    ssize_t bytesRead;

    while ((bytesRead = read(fileFd, buffer, sizeof(buffer))) > 0) {
        _socket.sendData(buffer, bytesRead);
    }

    close(fileFd);

    if (receiveResponse().find("200 OK") == 0) {
        std::cout << "Upload complete: " << filename << std::endl;
    } else {
        std::cout << "Error: Upload failed." << std::endl;
    }
}


void Client::listFiles() const {
    _socket.sendData("LIST");
    std::cout << "\n" << receiveResponse();
}


void Client::getFile(const std::string& filename) const {
    _socket.sendData(("GET " + filename).c_str());
    std::cout << std::endl;
    downloadFile(filename);
}


void Client::putFile(const std::string& filename) const {
    const int fileFd = open(("../../client/files/" + filename).c_str(), O_RDONLY);
    if (fileFd == -1) {
        std::cout << "Error: File not found on client: " << filename << std::endl;
        return;
    }
    _socket.sendData(("PUT " + filename).c_str());
    std::cout << std::endl;
    uploadFile(filename, fileFd);
}


void Client::deleteFile(const std::string& filename) const {
    _socket.sendData(("DELETE " + filename).c_str());;
    std::cout << "\n" << receiveResponse() << std::endl;
}


void Client::getFileInfo(const std::string& filename) const {
    _socket.sendData(("INFO " + filename).c_str());
    std::cout << "\n" << receiveResponse() << std::endl;
}

// overload method
