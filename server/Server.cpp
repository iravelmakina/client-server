// ReSharper disable CppExpressionWithoutSideEffects
#include "Server.h"

#include <iostream>
#include <thread>

#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/fcntl.h>


Server::Server(const std::string& directory) : _directory(directory) {}


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
        } else {
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
}


Socket Server::acceptClient() const {
    sockaddr_in clientAddr{};
    socklen_t clientAddrLen = sizeof(clientAddr);
    std::cout << "Waiting for a client to connect..." << std::endl;

    int clientfd = _serverSocket.acceptS(&clientAddr, &clientAddrLen);
    Socket clientSocket;
    clientSocket.setS(clientfd);

    std::cout << "Client connected!" << std::endl;
    return clientSocket;
}


void Server::handleClient(const Socket& clientSocket) const {
    while (true) {
        char buffer[1024] = {};
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
            clientSocket.sendData("Goodbye!");
            break;
        } else {
            clientSocket.sendData("400 BAD_REQUEST Invalid command.");
        }
    }
} // validation of len


void Server::handleGet(const Socket& clientSocket, const std::string& filename) const {
    const std::string filePath = _directory + filename;
    const int fileFd = open(filePath.c_str(), O_RDONLY);

    if (fileFd == -1) {
        clientSocket.sendData("404 NOT_FOUND File Not Found.");
        return;
    }

    clientSocket.sendData("200 OK");

    char ackBuffer[16] = {};
    clientSocket.receiveData(ackBuffer, sizeof(ackBuffer));
    if (std::string(ackBuffer) != "ACK") {
        std::cout << "Client did not acknowledge 200 OK\n";
        return;
    }

    struct stat fileStat{};
    fstat(fileFd, &fileStat);
    const uint32_t fileSize = fileStat.st_size;
    clientSocket.sendData(fileSize);

    clientSocket.receiveData(ackBuffer, sizeof(ackBuffer));
    if (std::string(ackBuffer) != "ACK") {
        std::cout << "Client did not acknowledge file size\n";
        return;
    }

    char buffer[1024];
    ssize_t bytesRead;

    while ((bytesRead = read(fileFd, buffer, sizeof(buffer))) > 0) {
        send(clientSocket.getS(), buffer, bytesRead, 0);
    } // correct
    close(fileFd);
    std::cout << "File sent successfully: " << filename << std::endl;
}


void Server::handlePut(const Socket& clientSocket, const std::string &filename) const {
    const int fileFd = open((_directory + filename).c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fileFd == -1) {
        clientSocket.sendData("500 SERVER_ERROR Unable to create file.");
        return;
    }

    clientSocket.sendData("200 OK");

    uint32_t fileSize;
    clientSocket.receiveData(fileSize);
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


void Server::handleList(const Socket& clientSocket) const {
    DIR* dir = opendir(_directory.c_str());
    if (!dir) {
        clientSocket.sendData("500 SERVER_ERROR Failed to open directory.");
        return;
    }

    dirent* entry;
    std::ostringstream fileListStream;


    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_REG) {
            fileListStream << entry->d_name << "\n";
        }
    }

    closedir(dir);
    clientSocket.sendData((fileListStream.str()).c_str());
}


void Server::handleDelete(const Socket& clientSocket, const std::string& filename) const {
    const std::string filePath = _directory + filename;

    if (access(filePath.c_str(), F_OK) == 0) {
        if (unlink(filePath.c_str()) == 0) {
            clientSocket.sendData("File deleted successfully.");
        } else {
            clientSocket.sendData("500 SERVER_ERROR Could not delete file.");
        }
    } else {
        clientSocket.sendData("404 NOT_FOUND File does not exist.");
    }
}


void Server::handleInfo(const Socket& clientSocket, const std::string& filename) const {
    const std::string filePath = _directory + filename;
    struct stat fileStat;

    if (access(filePath.c_str(), F_OK) == 0) {
        if (stat(filePath.c_str(), &fileStat) == 0) {
            std::ostringstream metadataStream;
            metadataStream << "Size: " << fileStat.st_size << " bytes\n";
            metadataStream << "Last Modified: " << ctime(&fileStat.st_mtime);
            metadataStream << "Last Accessed: " << ctime(&fileStat.st_atime);
            metadataStream << "Creation Time: " << ctime(&fileStat.st_birthtime);
            metadataStream << "Permissions: " << getFilePermissions(fileStat.st_mode) << "\n";
            clientSocket.sendData((metadataStream.str()).c_str());
        } else {
            clientSocket.sendData("500 SERVER_ERROR Unable to retrieve file info.");
        }
    } else {
        clientSocket.sendData("404 NOT_FOUND File does not exist.");
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