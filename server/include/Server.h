#pragma once

#include "Socket.h"


class Server {
public:
    explicit Server(const std::string &directory);

    void start(int port);
    void stop();

    void handleList(const Socket &clientSocket, const std::string &username) const;
    void handleGet(const Socket &clientSocket, const std::string &username, const std::string &filename) const;
    void handlePut(const Socket &clientSocket, const std::string &username, const std::string &filename) const;
    void handleDelete(const Socket &clientSocket, const std::string &username, const std::string &filename) const;
    void handleInfo(const Socket &clientSocket,  const std::string &username, const std::string &filename) const;

private:
    Socket _serverSocket;
    const std::string _directory;

    void run() const;

    Socket acceptClient() const;

    void handleClient(Socket clientSocket) const;

    static std::string receiveUsername(const Socket &clientSocket) ;

    void processCommands(Socket &clientSocket, const std::string &username) const;

    static std::string getFilePermissions(mode_t mode);

    static bool isValidFilename(const Socket &clientSocket, const std::string &filename);

    bool createClientFolderIfNotExists(const std::string &clientName) const;

    static void cleanupClient(Socket &clientSocket, const std::string &username) ;
};
