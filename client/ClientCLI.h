#pragma once

#include "Client.h"
#include <vector>


class ClientCLI {
public:
    ClientCLI();

    void run(const char *serverIp, int port);

private:
    Client client;

    static void printMenu();

    static std::vector<std::string> parseInput(const std::string &input);
};
