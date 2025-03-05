#include "Server.h"
#include "ThreadPool.h"

#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <thread>
#include <sys/fcntl.h>


Server::Server(const std::string &directory, const size_t maxSimultaneousClients) : _directory(directory),
    _threadPool(maxSimultaneousClients), _maxSimultaneousClients(maxSimultaneousClients) {
}


Server::~Server() {
    stop();
}


void Server::start(const int port) {
    if (!_serverSocket.createS()) {
        return;
    }

    if (!_serverSocket.bindS(port) || !_serverSocket.listenS(SOMAXCONN)) {
        _serverSocket.closeS();
        return;
    }
    std::cout << "Server listening on port " << port << std::endl;
    run();
}


void Server::stop() {
    _serverSocket.closeS();
    _threadPool.shutdown();
    std::cout << "Server stopped." << std::endl;
}


void Server::run() {
    while (true) {
        Socket clientSocket = acceptClient();
        if (clientSocket.getS() != -1) {
            _threadPool.submit([this, clientSocket] { handleClient(clientSocket); });
        }
    }
}


Socket Server::acceptClient() const {
    sockaddr_in clientAddr{};
    socklen_t clientAddrLen = sizeof(clientAddr);

    const int clientFd = _serverSocket.acceptS(&clientAddr, &clientAddrLen);

    Socket clientSocket(clientFd);

    if (_threadPool.activeThreads() >= _maxSimultaneousClients) {
        clientSocket.sendData("503 SERVICE UNAVAILABLE: Server is busy. Please try again later.");
        clientSocket.closeS();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        return clientSocket;
    }

    std::cout << "Client connected." << std::endl;
    clientSocket.sendData(RESPONSE_OK.c_str());

    return clientSocket;
}


void Server::handleClient(Socket clientSocket) const {
    const std::string username = receiveUsername(clientSocket);
    if (username.empty()) {
        clientSocket.sendData("400 BAD REQUEST: Invalid username.");
        cleanupClient(clientSocket, username);
        return;
    }

    if (!createClientFolderIfNotExists(username)) {
        clientSocket.sendData("500 SERVER ERROR: Unable to create client folder.");
        cleanupClient(clientSocket, username);
        return;
    }

    clientSocket.sendData(RESPONSE_OK.c_str());

    processCommands(clientSocket, username);
    cleanupClient(clientSocket, username);
}


void Server::processCommands(Socket &clientSocket, const std::string &username) const {
    char buffer[MESSAGE_SIZE] = {};

    while (true) {
        ssize_t bytesReceived = clientSocket.receiveData(buffer, sizeof(buffer));
        if (bytesReceived <= 0) {
            break;
        }

        buffer[bytesReceived] = '\0';

        std::string command(buffer);
        std::cout << "Received command from " << username << ": " << command << std::endl;

        std::istringstream stream(command);
        std::string action, filename;

        stream >> action;

        if (action == "INFO" || action == "GET" || action == "PUT" || action == "DELETE") {
            stream >> filename;
            if (!isValidFilename(filename)) {
                clientSocket.sendData("400 BAD REQUEST: Invalid filename.");
                continue;
            }
        }

        if (action == "GET") {
            handleGet(clientSocket, username, filename);
        } else if (action == "LIST") {
            handleList(clientSocket, username);
        } else if (action == "PUT") {
            handlePut(clientSocket, username, filename);
        } else if (action == "DELETE") {
            handleDelete(clientSocket, username, filename);
        } else if (action == "INFO") {
            handleInfo(clientSocket, username, filename);
        } else if (action == "EXIT") {
            break;
        } else {
            clientSocket.sendData("400 BAD REQUEST: Invalid command.");
        }
    }
}


void Server::handleGet(const Socket &clientSocket, const std::string &username, const std::string &filename) const {
    const std::string filePath = _directory + username + "/" + filename;
    const int fileFd = open(filePath.c_str(), O_RDONLY);
    if (fileFd == -1) {
        perror("open");
        clientSocket.sendData("404 NOT FOUND: File does not exist.");
        return;
    }

    clientSocket.sendData(RESPONSE_OK.c_str());

    char ackBuffer[3] = {};
    const ssize_t bytesReceived = clientSocket.receiveData(ackBuffer, sizeof(ackBuffer));
    if (bytesReceived <= 0) {
        close(fileFd);
        return;
    }

    if (std::string(ackBuffer) != RESPONSE_ACK) {
        std::cout << "\033[31m" << "Client did not acknowledge 200 OK." << "\033[0m" << std::endl;
        return;
    }

    char buffer[FILE_BUFFER_SIZE];
    ssize_t bytesRead;
    while ((bytesRead = read(fileFd, buffer, sizeof(buffer))) > 0) {
        clientSocket.sendData(buffer, bytesRead);
    }
    clientSocket.sendData("", 0);
    close(fileFd);
}


void Server::handlePut(const Socket &clientSocket, const std::string &username, const std::string &filename) const {
    const int fileFd = open((_directory + username + "/" + filename).c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fileFd == -1) {
        perror("open");
        clientSocket.sendData("500 SERVER ERROR: Unable to create file.");
        return;
    }

    clientSocket.sendData(RESPONSE_OK.c_str());

    char buffer[FILE_BUFFER_SIZE];
    ssize_t bytesReceived;
    while (true) {
        if ((bytesReceived = clientSocket.receiveData(buffer, sizeof(buffer))) <= 0) {
            close(fileFd);
            break;
        }
        write(fileFd, buffer, bytesReceived);
    }

    close(fileFd);
    clientSocket.sendData(RESPONSE_OK.c_str());
}


void Server::handleList(const Socket &clientSocket, const std::string &username) const {
    DIR *dir = opendir((_directory + username).c_str());
    if (!dir) {
        perror("opendir");
        clientSocket.sendData("500 SERVER ERROR: Failed to open directory.");
        return;
    }

    dirent *entry;
    std::ostringstream fileListStream;
    bool firstEntry = true;
    bool filesFound = false;

    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type == DT_REG) {
            if (!firstEntry) {
                fileListStream << "\n";
            }
            fileListStream << entry->d_name;
            firstEntry = false;
            filesFound = true;
        }
    }

    closedir(dir);

    if (!filesFound) {
        clientSocket.sendData("204 NO CONTENT: The directory is empty.");
    } else {
        clientSocket.sendData(fileListStream.str().c_str());
    }
}


void Server::handleDelete(const Socket &clientSocket, const std::string &username, const std::string &filename) const {
    const std::string filePath = _directory + username + "/" + filename;

    if (access(filePath.c_str(), F_OK) == 0) {
        if (unlink(filePath.c_str()) == 0) {
            clientSocket.sendData(RESPONSE_OK.c_str());
        } else {
            perror("unlink");
            clientSocket.sendData("500 SERVER ERROR: Unable to delete file.");
        }
    } else {
        perror("access");
        clientSocket.sendData("404 NOT FOUND: File does not exist.");
    }
}


void Server::handleInfo(const Socket &clientSocket, const std::string &username, const std::string &filename) const {
    const std::string filePath = _directory + username + "/" + filename;
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


std::string Server::receiveUsername(const Socket &clientSocket) {
    char buffer[MESSAGE_SIZE] = {};
    const ssize_t bytesReceived = clientSocket.receiveData(buffer, sizeof(buffer));
    if (bytesReceived <= 0) {
        return "";
    }

    const std::string username(buffer);
    for (const char c: username) {
        if (!isalnum(c) || isspace(c)) {
            return "";
        }
    }

    std::cout << "Client's name: " << username << std::endl;

    return username;
}


bool Server::isValidFilename(const std::string &filename) {
    if (filename.empty() || filename == "." || filename.find('/') != std::string::npos || filename.find('\\') !=
        std::string::npos) {
        return false;
    }
    return true;
}


bool Server::createClientFolderIfNotExists(const std::string &clientName) const {
    const std::string clientFolder = _directory + clientName;

    struct stat dirStat;

    if (stat(clientFolder.c_str(), &dirStat) != 0) {
        if (mkdir(clientFolder.c_str(), 0777) == -1) {
            perror("Error creating client folder");
            return false;
        }
    }
    return true;
}


void Server::cleanupClient(Socket &clientSocket, const std::string &username) {
    if (username.empty()) {
        std::cout << "Client disconnected." << std::endl;
    } else {
        std::cout << "Client " << username << " disconnected." << std::endl;
    }
    clientSocket.closeS();
}
