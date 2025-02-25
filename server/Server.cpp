// ReSharper disable CppExpressionWithoutSideEffects
#include "Server.h"

#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/fcntl.h>


Server::Server(const std::string &directory) : _directory(directory) {
}


void Server::start(const int port) {
    if (!_serverSocket.createS()) {
        return;
    }

    if (!_serverSocket.bindS(port) || !_serverSocket.listenS(1)) {
        _serverSocket.closeS();
        return;
    }
    std::cout << "Server listening on port " << port << std::endl;
    run();
}


void Server::stop() const {
    _serverSocket.closeS();
    std::cout << "Server stopped." << std::endl;
}


void Server::run() const {
    while (true) {
        Socket clientSocket = acceptClient();
        if (clientSocket.getS() != -1) {
            handleClient(clientSocket);
            clientSocket.closeS();
            std::cout << "Client disconnected." << std::endl;
        }
    }
}


Socket Server::acceptClient() const {
    sockaddr_in clientAddr{};
    socklen_t clientAddrLen = sizeof(clientAddr);
    std::cout << "\nWaiting for a client to connect..." << std::endl;

    int clientfd = _serverSocket.acceptS(&clientAddr, &clientAddrLen);

    const Socket clientSocket(clientfd);
    std::cout << "Client connected." << std::endl;
    clientSocket.sendData("200 OK");

    return clientSocket;
}


void Server::handleClient(const Socket &clientSocket) const {
    while (true) {
        char buffer[512] = {};
        const ssize_t bytesReceived = clientSocket.receiveData(buffer, sizeof(buffer) - 1);

        if (bytesReceived <= 0) {
            break;
        }

        std::string command(buffer);
        std::cout << "Received command: " << command << std::endl;

        if (command.find("GET") == 0) {
            handleGet(clientSocket, command.substr(4));
        } else if (command.find("LIST") == 0) {
            handleList(clientSocket);
        } else if (command.find("PUT") == 0) {
            handlePut(clientSocket, command.substr(4));
        } else if (command.find("DELETE") == 0) {
            handleDelete(clientSocket, command.substr(7));
        } else if (command.find("INFO") == 0) {
            handleInfo(clientSocket, command.substr(5));
        } else if (command == "EXIT") {
            break;
        } else {
            clientSocket.sendData("400 BAD_REQUEST Invalid command.");
        }
    }
}


void Server::handleGet(const Socket &clientSocket, const std::string &filename) const {
    const std::string filePath = _directory + filename;
    const int fileFd = open(filePath.c_str(), O_RDONLY);
    if (fileFd == -1) {
        perror("open");
        clientSocket.sendData("404 NOT FOUND: File does not exist.");
        return;
    }

    clientSocket.sendData("200 OK");

    char ackBuffer[32] = {};
    clientSocket.receiveData(ackBuffer, sizeof(ackBuffer));
    if (std::string(ackBuffer) != "ACK") {
        std::cout << "\033[31m" << "Client did not acknowledge 200 OK." << "\033[0m" << std::endl;
        return;
    }

    struct stat fileStat{};
    fstat(fileFd, &fileStat);
    const uint32_t fileSize = fileStat.st_size;
    clientSocket.sendData(fileSize);

    clientSocket.receiveData(ackBuffer, sizeof(ackBuffer));
    if (std::string(ackBuffer) != "ACK") {
        std::cout << "\033[31m" << "Client did not acknowledge file size." << "\033[0m" << std::endl;
        return;
    }

    char buffer[1024];
    ssize_t bytesRead;

    while ((bytesRead = read(fileFd, buffer, sizeof(buffer))) > 0) {
        clientSocket.sendData(buffer, bytesRead);
    }

    close(fileFd);
}


void Server::handlePut(const Socket &clientSocket, const std::string &filename) const {
    const int fileFd = open((_directory + filename).c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fileFd == -1) {
        perror("open");
        clientSocket.sendData("500 SERVER ERROR: Unable to create file.");
        return;
    }

    clientSocket.sendData("200 OK");

    uint32_t fileSize{};
    if (clientSocket.receiveData(fileSize) <= 0) {
        std::cout << "\033[31m" << "Failed to receive file size." << "\033[0m" << std::endl;
        return;
    }

    clientSocket.sendData("ACK");

    char buffer[1024];
    ssize_t bytesReceived;
    size_t totalReceived = 0;

    while (totalReceived < fileSize && (bytesReceived = clientSocket.receiveData(buffer, sizeof(buffer))) > 0) {
        write(fileFd, buffer, bytesReceived);
        totalReceived += bytesReceived;
    }

    close(fileFd);
    clientSocket.sendData("200 OK");
}


void Server::handleList(const Socket &clientSocket) const {
    DIR *dir = opendir(_directory.c_str());
    if (!dir) {
        perror("opendir");
        clientSocket.sendData("500 SERVER ERROR: Failed to open directory.");
        return;
    }

    dirent *entry;
    std::ostringstream fileListStream;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_REG) {
            fileListStream << entry->d_name << "\n";
        }
    }

    closedir(dir);
    clientSocket.sendData(fileListStream.str().c_str());
}


void Server::handleDelete(const Socket &clientSocket, const std::string &filename) const {
    const std::string filePath = _directory + filename;

    if (access(filePath.c_str(), F_OK) == 0) {
        if (unlink(filePath.c_str()) == 0) {
            clientSocket.sendData("200 OK");
        } else {
            perror("unlink");
            clientSocket.sendData("500 SERVER ERROR: Unable to delete file.");
        }
    } else {
        perror("access");
        clientSocket.sendData("404 NOT FOUND: File does not exist.");
    }
}


void Server::handleInfo(const Socket &clientSocket, const std::string &filename) const {
    const std::string filePath = _directory + filename;
    struct stat fileStat;

    if (access(filePath.c_str(), F_OK) == 0) {
        if (stat(filePath.c_str(), &fileStat) == 0) {
            std::ostringstream metadataStream;
            metadataStream << "Size: " << fileStat.st_size << " bytes\n";
            metadataStream << "Last Modified: " << ctime(&fileStat.st_mtime);
            metadataStream << "Last Accessed: " << ctime(&fileStat.st_atime);
            metadataStream << "Creation Time: " << ctime(&fileStat.st_birthtime);
            metadataStream << "Permissions: " << getFilePermissions(fileStat.st_mode);
            clientSocket.sendData(metadataStream.str().c_str());
        } else {
            perror("stat");
            clientSocket.sendData("500 SERVER ERROR: Unable to retrieve file info.");
        }
    } else {
        perror("access");
        clientSocket.sendData("404 NOT FOUND: File does not exist.");
    }
}


std::string Server::getFilePermissions(const mode_t mode) {
    std::ostringstream permissions;

    permissions << ((mode & S_IRUSR) ? "r" : "-");
    permissions << ((mode & S_IWUSR) ? "w" : "-");
    permissions << ((mode & S_IXUSR) ? "x" : "-");

    permissions << ((mode & S_IRGRP) ? "r" : "-");
    permissions << ((mode & S_IWGRP) ? "w" : "-");
    permissions << ((mode & S_IXGRP) ? "x" : "-");

    permissions << ((mode & S_IROTH) ? "r" : "-");
    permissions << ((mode & S_IWOTH) ? "w" : "-");
    permissions << ((mode & S_IXOTH) ? "x" : "-");

    return permissions.str();
}
