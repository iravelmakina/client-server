#pragma once

#include "Client.h"
#include <vector>


class ClientCLI {
public:
    explicit ClientCLI(const std::string &directory);

    void run(const char *serverIp, int port);

private:
    Client client;

    static void printMenu();

    static std::vector<std::string> parseInput(const std::string &input);

    static bool isValidUsername(const std::string &username);

    static std::string getUsernameFromUser();
};
