#pragma once

#include <string>
#include <netinet/in.h>


class Socket {
public:
    explicit Socket(int socketFd = -1);


    bool createS();
    void closeS() const;


    bool bindS(int port) const;
    bool listenS(int backlog) const;


    int acceptS(sockaddr_in *clientAddr, socklen_t *clientLen) const;
    bool connectS(const char *serverIp, int port) const;


    template<typename T>
    ssize_t sendData(const T &data) const {
        return send(_socketFd, &data, sizeof(T), 0);
    }

    template<typename T>
    ssize_t receiveData(T &data) const {
        return recv(_socketFd, &data, sizeof(T), MSG_WAITALL);
    }


    ssize_t sendData(const char *data, size_t dataLen = std::string::npos) const;
    ssize_t receiveData(char *data, size_t dataLen) const;


    int getS() const;
    void setS(int s);

private:
    int _socketFd;
};
