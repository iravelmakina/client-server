#pragma once

#include "ThreadPool.h"
#include "Socket.h"


enum class ReceiveStatus {
    SUCCESS,
    TIMEOUT,
    CLIENT_DISCONNECTED,
    ERROR
};

struct ReceiveResult {
    ReceiveStatus status;
    std::string message;
    ssize_t bytesReceived;
};


class Server {
public:
    explicit Server(const std::string &directory, size_t maxSimultaneousClients);

    void start(int port);
    void stop();

    void handleList(const Socket &clientSocket, const std::string &username) const;
    size_t handleGet(const Socket &clientSocket, const std::string &username, const std::string &filename) const;
    size_t handlePut(const Socket &clientSocket, const std::string &username, const std::string &filename) const;
    void handleDelete(const Socket &clientSocket, const std::string &username, const std::string &filename) const;
    void handleInfo(const Socket &clientSocket,  const std::string &username, const std::string &filename) const;

    ~Server();

private:
    Socket _serverSocket;
    const std::string _directory;
    ThreadPool _threadPool;
    size_t _maxSimultaneousClients;

    void run();

    Socket acceptClient() const;

    void defineVersionAndHandleClient(Socket clientSocket) const;

    void handleClient1dot0(Socket clientSocket) const;


    void handleClient2dot0(Socket clientSocket) const;

    static bool authenticateClient(const Socket &clientSocket, std::string &username) ;
    void processCommands(Socket &clientSocket, std::string &username) const;
    static void cleanupClient(Socket &clientSocket, const char* username = nullptr);

    static ReceiveResult receiveMessage(const Socket &clientSocket, char *buffer, size_t bufferSize, const char *username = nullptr);


    static bool isValidUsername(const std::string &username);

    static bool isValidFilename(const std::string &filename);

    bool createClientFolderIfNotExists(const std::string &clientName) const;

    static std::string getFilePermissions(mode_t mode);
};
