#pragma once

#include <string>
#include <netinet/in.h>


// Constants for response messages
const std::string RESPONSE_OK = "200 OK";
const std::string RESPONSE_ACK = "ACK";

// Constants for buffer sizes
constexpr int FILE_BUFFER_SIZE = 1024;
constexpr int MESSAGE_SIZE = 512;


class Socket {
public:
    explicit Socket(int socketFd = -1);


    bool createS();
    void closeS();


    bool bindS(int port) const;
    bool listenS(int backlog) const;


    int acceptS(sockaddr_in *clientAddr, socklen_t *clientLen) const;

    void shutdownS();

    bool connectS(const char *serverIp, int port) const;


    ssize_t sendData(const char *data, size_t dataLen = std::string::npos) const;
    ssize_t receiveData(char *buffer, size_t bufferSize) const;

    bool setRecvTimeout() const;


    int getS() const;
    void setS(int s);

    void setTimeoutSeconds(int timeoutSeconds);

private:
    int _socketFd;
    int _timeoutSeconds{-1};
    bool _shutdownFlag{false};
};
