#pragma once

#include "../include/common.h"

class Overseer {
public:
    int socket;
    bool connection_alive = true;
    std::string overseer_id;

    Overseer(int socket, std::string overseer_id);
    Overseer();
};
