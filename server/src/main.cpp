#include "Server.h"


int main() {
    Server server("files/", 2);
    server.start(9080);
    return 0;
}
