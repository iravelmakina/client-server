#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>

#include "Socket.h"

#include <cstdio>


Socket::Socket() : _socketfd(-1) {}


bool Socket::createS() {
    _socketfd = socket(AF_INET, SOCK_STREAM, 0);
    if (_socketfd == -1) {
        perror("Error creating socket");
        return false;
    }
    return true;
}


void Socket::closeS() const {
    if (_socketfd != -1) {
        close(_socketfd);
    }
}


bool Socket::bindS(int port) const {
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(_socketfd, reinterpret_cast<struct sockaddr*>(&serverAddr), sizeof(serverAddr)) == -1) {
        perror("Bind failed");
        return false;
    }
    return true;
}


bool Socket::listenS(const int backlog) const {
    if (listen(_socketfd, backlog) == -1) {
        perror("Listen failed");
        return false;
    }
    return true;
}


int Socket::acceptS(sockaddr_in* clientAddr, socklen_t* clientLen) const {
    int clientSocket = accept(_socketfd,
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

    if (connect(_socketfd,
                reinterpret_cast<struct sockaddr*>(&serverAddr),
                sizeof(serverAddr)) == -1) {
        perror("Connect failed");
        return false;
                }
    return true;
}


ssize_t Socket::sendData(const char* data) const {
    const ssize_t bytesSent = send(_socketfd, data, strlen(data), 0);
    return bytesSent;
}


ssize_t Socket::receiveData(char* buffer, size_t bufferSize) const {
    const ssize_t bytesReceived =  recv(_socketfd, buffer, bufferSize - 1, 0);
    return bytesReceived;
}


int Socket::getS() const { return _socketfd; }


void Socket::setS(const int s) { _socketfd = s; }
