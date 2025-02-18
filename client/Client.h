#pragma once

#include <Socket.h>


class Client {
public:
    void connect(const char* serverIp, int port);

    void disconnect() const;

    void sendCommand(const char* command) const;

    void receiveResponse() const;

private:
    Socket clientSocket;
};
