#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>

#include "Socket.h"

#include <cstdio>


Socket::Socket(const int socketFd) : _socketFd(socketFd) {}


bool Socket::createS() {
    _socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_socketFd == -1) {
        perror("Error creating socket");
        return false;
    }
    return true;
}


void Socket::closeS() const {
    if (_socketFd != -1) {
        close(_socketFd);
    }
}


bool Socket::bindS(int port) const {
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(_socketFd, reinterpret_cast<sockaddr*>(&serverAddr), sizeof(serverAddr)) == -1) {
        perror("Bind failed");
        return false;
    }
    return true;
}


bool Socket::listenS(const int backlog) const {
    if (listen(_socketFd, backlog) == -1) {
        perror("Listen failed");
        return false;
    }
    return true;
}


int Socket::acceptS(sockaddr_in* clientAddr, socklen_t* clientLen) const {
    const int clientSocket = accept(_socketFd,
                                    reinterpret_cast<struct sockaddr*>(clientAddr),
                                    clientLen);
    if (clientSocket == -1) {
        perror("Accept failed");
        return -1;
    }
    return clientSocket;
}


bool Socket::connectS(const char* serverIp, int port) const {
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, serverIp, &serverAddr.sin_addr);

    if (connect(_socketFd,
                reinterpret_cast<sockaddr*>(&serverAddr),
                sizeof(serverAddr)) == -1) {
        perror("Connect failed");
        return false;
                }
    return true;
}


ssize_t Socket::sendData(const char* data, size_t dataLen) const {
    if (dataLen == std::string::npos) {
        dataLen = strlen(data);
    }
    return send(_socketFd, data, dataLen, 0);
}


ssize_t Socket::receiveData(char* data, const size_t dataLen) const {
    const ssize_t bytesReceived = recv(_socketFd, data, dataLen - 1, 0);
    return bytesReceived;
}


int Socket::getS() const { return _socketFd; }


void Socket::setS(const int s) { _socketFd = s; }
