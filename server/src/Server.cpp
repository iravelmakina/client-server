#include "Server.h"
#include "ThreadPool.h"

#include <iostream>
#include <sstream>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <thread>
#include <sys/fcntl.h>


const std::vector<std::string> COMMANDS = {"GET", "PUT", "LIST", "DELETE", "INFO", "EXIT"};


Server::Server(const std::string &directory, const size_t maxSimultaneousClients) : _directory(directory),
    _threadPool(maxSimultaneousClients), _maxSimultaneousClients(maxSimultaneousClients) {
    for (const std::string &command: COMMANDS) {
        _commandStatistics[command] = 0;
    }
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


void Server::shutdown() {
    _stopFlag = true;
    _serverSocket.shutdownS();
    _serverSocket.closeS();
    _threadPool.shutdown();
    std::cout << "Server stopped." << std::endl;
    displayCommandStatistics();
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


size_t Server::handleGet(const Socket &clientSocket, const std::string &username, const std::string &filename) const {
    const std::string filePath = _directory + username + "/" + filename;
    const int fileFd = open(filePath.c_str(), O_RDONLY);
    if (fileFd == -1) {
        perror("open");
        clientSocket.sendData("404 NOT FOUND: File does not exist.");
        return 0;
    }

    clientSocket.sendData(RESPONSE_OK.c_str());

    char ackBuffer[3] = {};
    const ReceiveResult result = receiveMessage(clientSocket, ackBuffer, sizeof(ackBuffer), username.c_str());
    if (result.status != ReceiveStatus::SUCCESS) {
        std::cout << "\033[31m" << result.message << "\033[0m" << std::endl;
        close(fileFd);
        return -1;
    }

    if (std::string(ackBuffer) != RESPONSE_ACK) {
        std::cout << "\033[31m" << "Client did not acknowledge 200 OK." << "\033[0m" << std::endl;
        return 0;
    }

    char buffer[FILE_BUFFER_SIZE];
    ssize_t bytesRead;
    while ((bytesRead = read(fileFd, buffer, sizeof(buffer))) > 0) {
        clientSocket.sendData(buffer, bytesRead);
    }
    clientSocket.sendData("", 0);
    close(fileFd);
    return 0;
}


size_t Server::handlePut(const Socket &clientSocket, const std::string &username, const std::string &filename) const {
    const int fileFd = open((_directory + username + "/" + filename).c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fileFd == -1) {
        perror("open");
        clientSocket.sendData("500 SERVER ERROR: Unable to create file.");
        return 0;
    }

    clientSocket.sendData(RESPONSE_OK.c_str());

    while (true) {
        char buffer[FILE_BUFFER_SIZE];
        const ReceiveResult result = receiveMessage(clientSocket, buffer, sizeof(buffer), username.c_str());
        if (result.status == ReceiveStatus::ERROR || result.status == ReceiveStatus::TIMEOUT) {
            std::cout << "\033[31m" << result.message << "\033[0m" << std::endl;
            close(fileFd);
            return -1;
        }
        if (result.bytesReceived == 0) {
            break;
        }
        write(fileFd, buffer, result.bytesReceived);
    }

    close(fileFd);
    clientSocket.sendData(RESPONSE_OK.c_str());
    return 0;
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
    struct stat fileStat{};

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


Server::~Server() {
    if (!_stopFlag) {
        shutdown();
    }
}


void Server::run() {
    while (!_stopFlag) {
        Socket clientSocket = acceptClient();
        if (clientSocket.getS() != -1) {
            std::cout << "Client connected." << std::endl;
            _threadPool.submit([this, &clientSocket] { defineVersionAndHandleClient(clientSocket); });
        }
    }
}


Socket Server::acceptClient() const {
    sockaddr_in clientAddr{};
    socklen_t clientAddrLen = sizeof(clientAddr);

    const int clientFd = _serverSocket.acceptS(&clientAddr, &clientAddrLen);
    if (clientFd == -1) {
        return Socket(-1);
    }

    Socket clientSocket(clientFd);
    clientSocket.setTimeoutSeconds(600);

    if (_threadPool.activeThreads() >= _maxSimultaneousClients) {
        clientSocket.sendData("503 SERVICE UNAVAILABLE: Server is busy. Please try again later.");
        clientSocket.closeS();
        return clientSocket;
    }

    clientSocket.sendData(RESPONSE_OK.c_str());

    return clientSocket;
}

void Server::defineVersionAndHandleClient(Socket &clientSocket) {
    char buffer[MESSAGE_SIZE] = {};
    const ReceiveResult result = receiveMessage(clientSocket, buffer, sizeof(buffer));
    if (result.status != ReceiveStatus::SUCCESS) {
        std::cout << "\033[31m" << result.message << "\033[0m" << std::endl;
        cleanupClient(clientSocket);
        return;
    }

    const std::string initialMessage(buffer);
    std::string username;

    if (initialMessage == "2.0") {
        clientSocket.sendData(RESPONSE_OK.c_str());
        handleClient2dot0(clientSocket);
    } else if (initialMessage == "1.0") {
        clientSocket.sendData(RESPONSE_OK.c_str());
        handleClient1dot0(clientSocket);
    } else {
        clientSocket.sendData("400 BAD REQUEST: Invalid version.");
        std::cout << "\033[31m" << "Invalid version." << "\033[0m" << std::endl;
        cleanupClient(clientSocket);
    }
}


void Server::handleClient1dot0(Socket &clientSocket) {
    std::string username = "v1dot0";
    processCommands(clientSocket, username);
    cleanupClient(clientSocket, username.c_str());
}


void Server::handleClient2dot0(Socket &clientSocket) {
    std::string username;
    if (!authenticateClient(clientSocket, username)) {
        cleanupClient(clientSocket);
        return;
    }

    if (!createClientFolderIfNotExists(username)) {
        clientSocket.sendData("500 SERVER ERROR: Unable to create client folder.");
        cleanupClient(clientSocket, username.c_str());
        return;
    }
    clientSocket.sendData(RESPONSE_OK.c_str());

    processCommands(clientSocket, username);
    cleanupClient(clientSocket, username.c_str());
}


bool Server::authenticateClient(const Socket &clientSocket, std::string &username) {
    char buffer[MESSAGE_SIZE] = {};
    const ReceiveResult result = receiveMessage(clientSocket, buffer, sizeof(buffer));
    if (result.status != ReceiveStatus::SUCCESS) {
        std::cout << "\033[31m" << result.message << "\033[0m" << std::endl;
        return false;
    }

    username = buffer;
    if (!isValidUsername(username)) {
        clientSocket.sendData("400 BAD REQUEST: Invalid username.");
        std::cout << "\033[31m" << "Invalid username." << "\033[0m" << std::endl;
        return false;
    }

    std::cout << "Client's name: " << username << std::endl;
    return true;
}


void Server::processCommands(const Socket &clientSocket, std::string &username) {
    char buffer[MESSAGE_SIZE] = {};
    while (true) {
        ReceiveResult result = receiveMessage(clientSocket, buffer, sizeof(buffer), username.c_str());
        if (result.status != ReceiveStatus::SUCCESS) {
            std::cout << "\033[31m" << result.message << "\033[0m" << std::endl;
            break;
        }

        buffer[result.bytesReceived] = '\0';
        std::string command(buffer);
        std::cout << "Received command from " << username << ": " << command << std::endl;

        std::istringstream stream(command);
        std::string action, filename;
        stream >> action;

        updateCommandStatistics(action);

        if (action == "INFO" || action == "GET" || action == "PUT" || action == "DELETE") {
            stream >> filename;
            if (!isValidFilename(filename)) {
                clientSocket.sendData("400 BAD REQUEST: Invalid filename.");
                return;
            }
        }

        if (action == "GET") {
            if (handleGet(clientSocket, username, filename) == -1) break;
        } else if (action == "LIST") {
            handleList(clientSocket, username);
        } else if (action == "PUT") {
            if (handlePut(clientSocket, username, filename) == -1) break;
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


void Server::cleanupClient(Socket &clientSocket, const char *username) {
    if (username == nullptr) {
        std::cout << "Closing socket of not authenticated client." << std::endl;
    } else {
        std::cout << "Closing socket of client " << username << "." << std::endl;
    }
    clientSocket.closeS();
}


ReceiveResult Server::receiveMessage(const Socket &clientSocket, char *buffer, const size_t bufferSize,
                                     const char *username) {
    ReceiveResult result;
    result.bytesReceived = clientSocket.receiveData(buffer, bufferSize);

    if (username == nullptr) {
        username = "not authenticated yet";
    }
    const std::string usernameStr(username);

    if (result.bytesReceived > 0) {
        result.status = ReceiveStatus::SUCCESS;
        result.message = "Data received successfully from client" + usernameStr + ".";
    } else if (result.bytesReceived == 0) {
        result.status = ReceiveStatus::CLIENT_DISCONNECTED;
        result.message = "Client " + usernameStr + " disconnected.";
    } else {
        switch (errno) {
            case EAGAIN:
                result.status = ReceiveStatus::TIMEOUT;
                result.message = "Receive timeout from client " + usernameStr + ".";
                break;
            case ECONNRESET:
                result.status = ReceiveStatus::CLIENT_DISCONNECTED;
                result.message = "Client " + usernameStr + " disconnected.";
                break;
            default:
                result.status = ReceiveStatus::ERROR;
                result.message = std::string("Receive error: ") + strerror(errno) + " from client " + usernameStr + ".";
                break;
        }
    }

    return result;
}


bool Server::isValidUsername(const std::string &username) {
    for (const char c: username) {
        if (!isalnum(c) || isspace(c)) {
            return false;
        }
    }
    return true;
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

    struct stat dirStat{};

    if (stat(clientFolder.c_str(), &dirStat) != 0) {
        if (mkdir(clientFolder.c_str(), 0777) == -1) {
            perror("Error creating client folder");
            return false;
        }
    }
    return true;
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


void Server::updateCommandStatistics(const std::string &command) {
    std::lock_guard<std::mutex> lock(_statisticsMutex);
    _commandStatistics[command]++;
}

void Server::displayCommandStatistics() const {
    std::lock_guard<std::mutex> lock(_statisticsMutex);
    std::cout << "\nCommand Statistics:" << std::endl;
    for (const std::pair<const std::string, int> &entry: _commandStatistics) {
        std::cout << entry.first << ": " << entry.second << " time(s)" << std::endl;
    }
}
