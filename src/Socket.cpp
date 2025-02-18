#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>

#include "Socket.h"


Socket::Socket() : _socketfd(-1) {}


bool Socket::createS() {
    _socketfd = socket(AF_INET, SOCK_STREAM, 0);
    return _socketfd != -1;
}


void Socket::closeS() const {
    if (_socketfd != -1) {
        close(_socketfd);
    }
}


bool Socket::sendData(const char* data) const {
    return send(_socketfd, data, strlen(data), 0) != -1;
}


bool Socket::receiveData() const {
    char buffer[1024];
    memset(buffer, 0, 1024);
    ssize_t bytes = recv(_socketfd, buffer, sizeof(buffer) - 1, 0);
    if (bytes > 0) {
        return true;
    }
    return false;
}


int Socket::getS() const { return _socketfd; }


void Socket::setS(const int s) { _socketfd = s; }
