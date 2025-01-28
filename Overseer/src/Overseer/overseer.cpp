#include "overseer.h"
#include <cstring>

Overseer::Overseer() {}

Overseer::~Overseer() {
    if (sock)
        close(sock);

    if (listen_thread.joinable()) {
        stop_listening = true;
        listen_thread.join();
    }
}

void Overseer::send_authentication_request() {
    char buffer[BUFFER_SIZE] = {0};
    buffer[0] = OS_AUTH;
    memcpy(buffer + 1, server_password.c_str(), server_password.size());
    send(sock, buffer, BUFFER_SIZE, 0);
    std::cout << "Sent authentication request to server...\n";
}

void Overseer::handle_response() {
    char buffer[BUFFER_SIZE] = {0};
    while (!stop_listening) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(sock, buffer, BUFFER_SIZE, 0);

        if (bytes_received <= 0) {
            std::cout << "Connection lost or no data received.\n";
            break;
        }

        uint8_t ptype = buffer[0];
        switch (ptype) {
            case OS_AUTH_INFO: handle_auth_info(buffer); break;
            case OS_INFORMER_INFO: handle_informer_info(buffer); break;
            case OS_UPDATE_USG: handle_update_usage(buffer); break;
            case OS_NOTIFY_INFORMER_TIMEOUT: handle_informer_timeout(buffer); break;
            case OS_PING: handle_ping(buffer); break;
            default: std::cout << "Received unknown packet type: " << (int)ptype << "\n"; break;
        }
    }
}

void Overseer::handle_auth_info(const char* buffer) {
    if (buffer[1] == 0) {
        std::cout << "Authentication successful.\n";
        authenticated = true;
    } else {
        std::cout << "Authentication failed: " << (buffer + 2) << "\n";
        authenticated = false;
    }
}

void Overseer::handle_informer_info(const char* buffer) {
    Informer informer;
    informer.informer_id = std::string(buffer + 1, 32);
    informer.info.computer_name = std::string(buffer + 33, 32);
    informer.info.platform = std::string(buffer + 65, 16);
    informer.info.cpu_model = std::string(buffer + 81, 32);
    informer.info.cores = buffer[113];
    informer.info.memory_gb = ntohs(*(uint16_t*)(buffer + 114));
    informer.info.storage_gb = ntohll(*(uint64_t*)(buffer + 118));

    informers[informer.informer_id] = informer;
    informer.display_system_information();

    emit informer_updated(informer.informer_id);
}

void Overseer::handle_update_usage(char* buffer) {
    uint64_t cpu, memory, network_upload, network_download, disk;
    std::string informer_id(buffer + 1, 32);
    memcpy(&cpu, buffer + 33, sizeof(uint64_t));
    memcpy(&memory, buffer + 41, sizeof(uint64_t));
    memcpy(&network_upload, buffer + 49, sizeof(uint64_t));
    memcpy(&network_download, buffer + 57, sizeof(uint64_t));
    memcpy(&disk, buffer + 65, sizeof(uint64_t));

    if (informers.find(informer_id) != informers.end()) {
        informers[informer_id].update_usage(cpu, memory, network_upload, network_download, disk);
    } else {
        std::cout << "Error: ID Not Found\nSystem Information not updated\n";
    }

    std::cout << "Received System Usage Information\n";
    informers[informer_id].display_system_usage();

    emit informer_updated(informer_id);
}

void Overseer::handle_ping(char* buffer) {
    buffer[0] = OS_PONG;
    send(sock, buffer, BUFFER_SIZE, 0);
}

void Overseer::handle_informer_timeout(char* buffer) {
    std::string informer_id(buffer + 1, 32);
    std::string reason(buffer + 33, 32);

    if (informers.find(informer_id) != informers.end()) {
        std::cout << "Informer " << informer_id << " Disconnected, Reason: " << reason << std::endl;
        informers.erase(informer_id);
    } else {
        std::cout << "Received unknown informer id to timeout." << std::endl;
    }

    emit informer_disconnected(informer_id);
}

bool Overseer::connect_to_server() {
    struct sockaddr_in server_addr;
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        std::cerr << "Error creating socket\n";
        return false;
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());

    if (::connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        std::cerr << "Error connecting to server\n";
        return false;
    }

    std::cout << "Connected to server at " << server_ip << ":" << server_port << std::endl;
    listen_thread = std::thread(&Overseer::handle_response, this);
    return true;
}

bool Overseer::authenticate_to_server() {
    for (int i = 0; i < 3; ++i) {
        send_authentication_request();
        std::this_thread::sleep_for(std::chrono::seconds(1));
        if (authenticated) return true;
    }
    return false;
}

void Overseer::set_server_ip(std::string ip) { server_ip = ip; }
void Overseer::set_server_port(int port) { server_port = port; }
void Overseer::set_server_password(std::string password) { server_password = password; }
