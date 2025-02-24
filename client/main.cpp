#include "Client.h"
#include <iostream>

#include "ClientCLI.h"

int main() {
    ClientCLI cli;
    cli.run("127.0.0.1", 9080);
    return 0;
}
