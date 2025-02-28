#pragma once

#include <string>
#include <netinet/in.h>


class Socket {
public:
    explicit Socket(int socketFd = -1);


    bool createS();
    void closeS();


    bool bindS(int port) const;
    bool listenS(int backlog) const;


    int acceptS(sockaddr_in *clientAddr, socklen_t *clientLen) const;
    bool connectS(const char *serverIp, int port) const;


    ssize_t sendData(const char *data, size_t dataLen = std::string::npos) const;
    ssize_t receiveData(char *buffer, size_t bufferSize) const;


    int getS() const;
    void setS(int s);

private:
    int _socketFd;
};
