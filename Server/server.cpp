#include <cstdint>
#include <iostream>
#include <netinet/in.h>
#include <thread>
#include <map>
#include <mutex>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <random>

#define PORT 8080
#define BUFFER_SIZE 128

// ------------------- Informer Class -------------------
class Informer {
public:
    struct SystemInformation {
        std::string computer_name;
        std::string platform;
        std::string cpu_model;
        uint8_t cores;
        uint16_t memory_gb;
        uint16_t swap_gb;
        uint64_t storage_gb;
    };

    struct SystemUsage {
        uint64_t cpu_usage;
        uint64_t memory_usage;
        uint64_t network_upload;
        uint64_t network_download;
        uint64_t disk_used_gb;
    };

    Informer() : socket(-1) {}
    Informer(int socket) : socket(socket) {}

    std::string informer_id;
    SystemInformation info;
    SystemUsage usage;
    int socket;

    void update_usage(uint64_t cpu, uint64_t memory, uint64_t network_upload, uint64_t network_download, uint64_t disk) {
        usage.cpu_usage = cpu;
        usage.memory_usage = memory;
        usage.network_upload = network_upload;
        usage.network_download = network_download;
        usage.disk_used_gb = disk;
    }

    void usage_to_lendian() {
        usage.cpu_usage = ntohll(usage.cpu_usage);
        usage.memory_usage = ntohll(usage.memory_usage);
        usage.network_upload = ntohll(usage.network_upload);
        usage.network_download = ntohll(usage.network_download);
        usage.disk_used_gb = ntohll(usage.disk_used_gb);
    }

    void sysinfo_to_lendian() {
        info.memory_gb = ntohs(info.memory_gb);
        info.swap_gb = ntohs(info.swap_gb);
        info.storage_gb = ntohll(info.storage_gb);
    }

    uint64_t ntohll( uint64_t val ) {
        val = ((val << 8) & 0xFF00FF00FF00FF00ULL ) | ((val >> 8) & 0x00FF00FF00FF00FFULL );
        val = ((val << 16) & 0xFFFF0000FFFF0000ULL ) | ((val >> 16) & 0x0000FFFF0000FFFFULL );
        return (val << 32) | (val >> 32);
    }



    void display_system_information() {
        std::cout << "Informer id: " << informer_id << std::endl;
        std::cout << "Computer Name: " << info.computer_name << std::endl;
        std::cout << "Platform: " << info.platform << std::endl;
        std::cout << "CPU Model: " << info.cpu_model << std::endl;
        printf("Number of Cores: %d\n", info.cores);
        printf("Memory: %u\n", info.memory_gb);
        printf("Swap Size: %u\n", info.swap_gb);
        printf("Total Storage: %lu\n", info.storage_gb);
    }

    void display_system_usage() {
        std::cout << "Informer id: " << informer_id << std::endl;
        printf("CPU Usage: %lu\n", usage.cpu_usage);
        printf("Memory Used: %lu\n", usage.memory_usage);
        printf("Network Up/Down: %lu/%lu\n", usage.network_upload, usage.network_download);
        printf("Disk space used: %lu\n", usage.disk_used_gb);
    }


};

// ------------------- NetMonServer Class -------------------

class NetMonServer {
private:
    int server_fd;
    std::map<std::string, Informer> informers;
    std::mutex informers_mutex;

    void initialize_socket() {
        struct sockaddr_in address;
        int opt = 1;

        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd == 0) {
            perror("Socket failed");
            exit(EXIT_FAILURE);
        }
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(PORT);

        if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
            perror("Bind failed");
            exit(EXIT_FAILURE);
        }

        if (listen(server_fd, 3) < 0) {
            perror("Listen failed");
            exit(EXIT_FAILURE);
        }

        std::cout << "Server listening on port " << PORT << "...\n";
    }

    void handle_client(int client_socket) {
        char buffer[BUFFER_SIZE];
        while (true) {
            memset(buffer, 0, BUFFER_SIZE);
            int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
            if (bytes_received <= 0) break;

            uint8_t ptype = buffer[0];
            switch (ptype) {
                case 0x00: // Informer Initialization
                    handle_informer_init(client_socket, buffer);
                    break;
                case 0x10: // Informer System Info
                    handle_system_info(client_socket, buffer);
                    break;
                default:
                    std::cout << "Unknown packet type from client.\n";
            }
        }
        close(client_socket);
    }

    void handle_informer_init(int client_socket, char* buffer) {
        Informer::SystemInformation info;

        info.computer_name = std::string(buffer+1, 32);
        info.platform = std::string(buffer+33, 16);
        info.cpu_model = std::string(buffer+49, 32);

        // Copy integers safely using memcpy
        memcpy(&info.cores, buffer + 81, sizeof(uint8_t));
        memcpy(&info.memory_gb, buffer + 82, sizeof(uint16_t));
        memcpy(&info.swap_gb, buffer + 84, sizeof(uint16_t));
        memcpy(&info.storage_gb, buffer + 86, sizeof(uint64_t));

        std::string informer_id = generate_random_id();
        Informer informer(client_socket);
        informer.info = info;
        informer.informer_id = informer_id;
        informer.sysinfo_to_lendian();

        {
            std::lock_guard<std::mutex> lock(informers_mutex);
            informers[informer_id] = informer;
        }

        char response[BUFFER_SIZE] = {0};
        response[0] = 0x01;
        response[1] = 0x00;  // No error
        memcpy(response + 2, informer_id.c_str(), informer_id.size());
        send(client_socket, response, BUFFER_SIZE, 0);

        std::cout << "Computer Initialized" << std::endl;
        informers[informer_id].display_system_information();
        std::cout << std::endl;
    }

    void handle_system_info(int client_socket, char* buffer) {
        uint8_t error_bit = 0x00;
        uint8_t ack_bit = 0x01;
        std::string error_msg = "";

        uint64_t memory_usage,network_upload, network_download, disk_used, cpu_usage;
        std::string informer_id(buffer + 1, 32);
        memcpy(&cpu_usage, buffer + 33 , sizeof(uint64_t));
        memcpy(&memory_usage, buffer + 41, sizeof(uint64_t));
        memcpy(&network_upload, buffer + 49, sizeof(uint64_t));
        memcpy(&network_download, buffer + 57, sizeof(uint64_t));
        memcpy(&disk_used, buffer + 65, sizeof(uint64_t));

        if (informers.find(informer_id) == informers.end()) {
            std::cout << "Error: ID Not Found" << std::endl;
            std::cout << "System Informaton not updated" << std::endl;

            return;
        }


        {
            std::lock_guard<std::mutex> lock(informers_mutex);
            if (informers.find(informer_id) != informers.end()) {
                informers[informer_id].update_usage(cpu_usage, memory_usage, network_upload, network_download, disk_used);
                informers[informer_id].usage_to_lendian();
            } else if (informers.find(informer_id) == informers.end()) {
                std::cout << "Error: ID Not Found" << std::endl;
                std::cout << "System Informaton not updated" << std::endl;
                error_bit = 0x01;
                ack_bit = 0x00;
                error_msg = "Error: ID Not Found";
            }

        }

        char response[BUFFER_SIZE] = {0};
        response[0] = 0x11;
        response[1] = error_bit;
        response[2] = ack_bit;
        if (error_msg.size() && error_msg.size() <= 32) {
            memcpy(response + 3, error_msg.c_str(), error_msg.size());
        }
        send(client_socket, response, BUFFER_SIZE, 0);
        std::cout << "Received System Usage Information" << std::endl;
        informers[informer_id].display_system_usage();
        std::cout << std::endl;
    }

    std::string generate_random_id() {
        const std::string characters = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, characters.size() - 1);

        std::string id;
        id.reserve(32);
        for (int i = 0; i < 32; ++i) {
            id += characters[dis(gen)];
        }

        return id;
    }

public:
    void run() {
        initialize_socket();

        while (true) {
            struct sockaddr_in address;
            socklen_t addrlen = sizeof(address);
            int client_socket = accept(server_fd, (struct sockaddr *)&address, &addrlen);
            if (client_socket < 0) {
                perror("Accept failed");
                continue;
            }

            std::cout << "New client connected.\n";
            std::thread(&NetMonServer::handle_client, this, client_socket).detach();
        }
    }
};

// ------------------- Main -------------------

int main() {
    NetMonServer server;
    server.run();
    return 0;
}
