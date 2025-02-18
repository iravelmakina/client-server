#pragma once

#include "Socket.h"

class Server {
public:
    void start(int port);

    void stop() const;

private:
    Socket serverSocket;

    void run() const;

    Socket acceptClient() const;

    static void handleClient(Socket clientSocket);
};
