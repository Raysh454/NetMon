#include "Server/server.h"

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cout << "Usage: ./Server <Overseer Password>" << std::endl;
        return 1;
    }

    NetMonServer server(argv[1]);
    server.run();
    return 0;
}

