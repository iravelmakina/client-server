#pragma once

#include <Socket.h>


enum class ReceiveStatus {
    SUCCESS,
    TIMEOUT,
    SERVER_DISCONNECTED,
    ERROR
};

struct ReceiveResult {
    ReceiveStatus status;
    std::string message;
    ssize_t bytesReceived;
};


class Client {
public:
    explicit Client(const std::string &directory);

    int connect(const char *serverIp, int port);

    int sendUsername(const std::string &username);

    void disconnect();

    bool isConnected() const;

    void listFiles();
    void getFile(const std::string &filename);
    void putFile(const std::string &filename);
    void deleteFile(const std::string &filename);
    void getFileInfo(const std::string &filename);

private:
    Socket _socket;
    const std::string _directory;

    std::string receiveResponse();

    void downloadFile(const std::string &filename);
    void uploadFile(const std::string &filename, int fileFd);
};
