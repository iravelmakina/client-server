#include "Server.h"


int main() {
    Server server("files/");
    server.start(9080);
    return 0;
}
