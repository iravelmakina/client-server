#pragma once

#include <Socket.h>
#include <string>


class Client {
public:
    void connect(const char* serverIp, int port);

    void disconnect() const;

    void listFiles() const;
    void getFile(const std::string& filename) const;
    void putFile(const std::string& filename) const;
    void deleteFile(const std::string& filename) const;
    void getFileInfo(const std::string& filename) const;

private:
    Socket clientSocket;

    void sendCommand(const char* command) const;

    void receiveResponse() const;
};
