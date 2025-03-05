#include "Socket.h"

#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>


Socket::Socket(const int socketFd) : _socketFd(socketFd) {
}


bool Socket::createS() {
    _socketFd = socket(AF_INET, SOCK_STREAM, 0);
    if (_socketFd == -1) {
        perror("Error creating socket");
        return false;
    }
    return true;
}


void Socket::closeS() {
    if (_socketFd != -1) {
        close(_socketFd);
    }
    _socketFd = -1;
}


bool Socket::bindS(int port) const {
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(port);

    if (bind(_socketFd, reinterpret_cast<sockaddr *>(&serverAddr), sizeof(serverAddr)) == -1) {
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


int Socket::acceptS(sockaddr_in *clientAddr, socklen_t *clientLen) const {
    const int clientSocket = accept(_socketFd,
                                    reinterpret_cast<struct sockaddr *>(clientAddr),
                                    clientLen);
    if (clientSocket == -1) {
        perror("Accept failed");
        return -1;
    }

    return clientSocket;
}


bool Socket::connectS(const char *serverIp, int port) const {
    sockaddr_in serverAddr{};
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(port);
    inet_pton(AF_INET, serverIp, &serverAddr.sin_addr);

    if (connect(_socketFd,
                reinterpret_cast<sockaddr *>(&serverAddr),
                sizeof(serverAddr)) == -1) {
        perror("Connect failed");
        return false;
    }
    return true;
}


ssize_t Socket::sendData(const char *data, size_t dataLen) const {
    if (dataLen == std::string::npos) {
        dataLen = strlen(data);
    }

    if (dataLen > UINT32_MAX) {
        return -1; // data too large to send
    }

    const uint32_t netDataLen = htonl(static_cast<uint32_t>(dataLen));
    const ssize_t sentBytes = send(_socketFd, &netDataLen, sizeof(netDataLen), 0);
    if (sentBytes != sizeof(netDataLen)) {
        return -1; // failed to send complete length prefix
    }

    return send(_socketFd, data, dataLen, 0);
}


ssize_t Socket::receiveData(char *buffer, const size_t bufferSize) const {
    if (!setRecvTimeout(600)) {
        return -1;
    }

    uint32_t netDataLen;
    const ssize_t receivedBytes = recv(_socketFd, &netDataLen, sizeof(netDataLen), MSG_WAITALL);
    if (receivedBytes != sizeof(netDataLen)) {
        return -1; // failed to receive complete length prefix
    }

    const uint32_t dataLen = ntohl(netDataLen);

    if (dataLen > bufferSize) {
        return -1; // buffer is too small for incoming data
    }

    return recv(_socketFd, buffer, dataLen, MSG_WAITALL);
}


bool Socket::setRecvTimeout(const int timeoutSeconds) const {
    struct timeval tv;
    tv.tv_sec = timeoutSeconds;
    tv.tv_usec = 0;

    if (setsockopt(_socketFd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv)) == -1) {
        perror("error setting receive timeout");
        return false;
    }
    return true;
}


int Socket::getS() const {
    return _socketFd;
}


void Socket::setS(const int s) {
    _socketFd = s;
}
