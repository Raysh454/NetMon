#include "overseer.h"

Overseer::Overseer(int socket, std::string overseer_id) : socket(socket), overseer_id(overseer_id) {}
Overseer::Overseer() : socket(-1), overseer_id("") {}

