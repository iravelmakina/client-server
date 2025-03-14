#include "Client.h"

#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>


Client::Client(const std::string &directory) : _directory(directory) {
}


int Client::connect(const char *serverIp, const int port) {
    if (!_socket.createS()) {
        return -1;
    }

    if (!_socket.connectS(serverIp, port)) {
        _socket.closeS();
        return -1;
    }

    const std::string connectionResponse = receiveResponse();
    if (connectionResponse != RESPONSE_OK) {
        std::cout << connectionResponse << std::endl;
        return -1;
    }

    _socket.sendData("2.0");
    const std::string versionResponse = receiveResponse();
    if (versionResponse != RESPONSE_OK) {
        std::cout << versionResponse << std::endl;
        return -1;
    }

    std::cout << "\nConnected to server at " << serverIp << ":" << port << "." << std::endl;
    return 0;
}


void Client::disconnect() {
    _socket.sendData("EXIT");
    _socket.closeS();
    std::cout << "\nDisconnected from server." << std::endl;
}


bool Client::isConnected() const {
    return _socket.getS() != -1;
}


int Client::sendUsername(const std::string& username) {
    _socket.sendData(username.c_str());

    const std::string response = receiveResponse();
    if (response != RESPONSE_OK) {
        std::cout << response << std::endl;
        return -1;
    }

    return 0;
}


void Client::listFiles() {
    _socket.sendData("LIST");
    std::cout << receiveResponse() << std::endl;
}


void Client::getFile(const std::string &filename) {
    _socket.sendData(("GET " + filename).c_str());
    downloadFile(filename);
}


void Client::putFile(const std::string &filename) {
    const int fileFd = open((_directory + filename).c_str(), O_RDONLY);
    if (fileFd == -1) {
        std::cout << "File not found on client." << std::endl;
        return;
    }
    _socket.sendData(("PUT " + filename).c_str());
    uploadFile(filename, fileFd);
}


void Client::deleteFile(const std::string &filename) {
    _socket.sendData(("DELETE " + filename).c_str());

    const std::string response = receiveResponse();
    if (response == RESPONSE_OK) {
        std::cout << "Delete complete." << std::endl;
    } else {
        std::cout << response << std::endl;
    }
}


void Client::getFileInfo(const std::string &filename) {
    _socket.sendData(("INFO " + filename).c_str());
    std::cout << receiveResponse() << std::endl;
}


std::string Client::receiveResponse() {
    char buffer[MESSAGE_SIZE] = {};
    const ssize_t bytesReceived = _socket.receiveData(buffer, sizeof(buffer));
    if (bytesReceived <= 0) {
        if (bytesReceived == 0) {
            std::cout << "\033[31m" << "Error: Server closed the connection." << "\033[0m" << std::endl;
        } else {
            std::cout << "\033[31m" << "Error: No response from server. Closing socket." << "\033[0m" << std::endl;
        }
        _socket.closeS();
        return "";
    }
    return {buffer};
}


void Client::downloadFile(const std::string &filename) {
    const std::string response = receiveResponse();
    if (response != RESPONSE_OK) {
        std::cout << response << std::endl;
        return;
    }

    _socket.sendData(RESPONSE_ACK.c_str());

    const int fileFd = open((_directory + filename).c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0666);
    if (fileFd == -1) {
        std::cout << "\033[31m" << "Error: Unable to create file." << "\033[0m" << std::endl;
        return;
    }

    char buffer[FILE_BUFFER_SIZE];
    ssize_t bytesReceived;
    while ((bytesReceived = _socket.receiveData(buffer, sizeof(buffer))) > 0) {
        write(fileFd, buffer, bytesReceived);
    }

    close(fileFd);
    std::cout << "Download complete: " << filename << std::endl;
}


void Client::uploadFile(const std::string &filename, const int fileFd) {
    const std::string response = receiveResponse();
    if (response != RESPONSE_OK) {
        std::cout << response << std::endl;
        return;
    }

    char buffer[FILE_BUFFER_SIZE];
    ssize_t bytesRead;
    while ((bytesRead = read(fileFd, buffer, sizeof(buffer))) > 0) {
        _socket.sendData(buffer, bytesRead);
    }

    _socket.sendData("", 0);
    close(fileFd);

    if (receiveResponse() == RESPONSE_OK) {
        std::cout << "Upload complete: " << filename << std::endl;
    } else {
        std::cout << "\033[31m" << "Error: Upload failed." << "\033[0m" << std::endl;
    }
}
