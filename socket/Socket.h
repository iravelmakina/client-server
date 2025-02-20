#pragma once
#include <string>
#include <netinet/in.h>

class Socket {
public:
    Socket();

    bool createS();

    void closeS() const;


    bool bindS(int port) const;

    bool listenS(int backlog) const;


    int acceptS(sockaddr_in* clientAddr, socklen_t* clientLen) const;
    bool connectS(const char* serverIp, int port) const;


    template <typename T>
    ssize_t sendData(const T& data) const {
        return send(_socketfd, &data, sizeof(T), 0);
    }


    template <typename T>
    ssize_t receiveData(T& data) const {
        return recv(_socketfd, &data, sizeof(T), MSG_WAITALL);
    }

    ssize_t sendData(const char* data) const;

    ssize_t receiveData(char* buffer, size_t bufferSize) const;


    int getS() const;

    void setS(int s);


private:
    int _socketfd;
};
