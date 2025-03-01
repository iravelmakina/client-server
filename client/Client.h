#pragma once

#include <Socket.h>

class Client {
public:
    explicit Client(const std::string &directory);

    int connect(const char *serverIp, int port);
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
