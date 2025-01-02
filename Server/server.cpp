#include <chrono>
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
#define UPDATE_OVERSEERS_INTERVAL 5
#define INFORMER_TIMEOUT 10
#define OVERSEER_TIMEOUT 10
#define CLEANUP_INFORMERS_INTERVAL 20
#define CLEANUP_OVERSEERS_INTERVAL 20

typedef enum {
    INF_INIT = 0b00,
    INF_APPROVE = 0b01,
    INF_USAGE = 0b10,
    INF_ACK = 0b11,
    OS_AUTH = 0b100,
    OS_AUTH_INFO = 0b101,
    OS_INFORMER_INFO = 0b110,
    OS_UPDATE_USG = 0b111,
    OS_NOTIFY_INFORMER_TIMEOUT = 0b1000,
    OS_PING = 0b1001,
    OS_PONG = 0b1010,
} PTYPE;

uint64_t ntohll( uint64_t val ) {
    val = ((val << 8) & 0xFF00FF00FF00FF00ULL ) | ((val >> 8) & 0x00FF00FF00FF00FFULL );
    val = ((val << 16) & 0xFFFF0000FFFF0000ULL ) | ((val >> 16) & 0x0000FFFF0000FFFFULL );
    return (val << 32) | (val >> 32);
}


// ------------------- Overseer Class -------------------

class Overseer {
    public:
        int socket;
        bool connection_alive = true;
        std::string overseer_id;

    Overseer(int socket, std::string overseer_id) : socket(socket), overseer_id(overseer_id) {};
    Overseer() : socket(-1), overseer_id("") {};
};

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

    Informer() : socket(-1) {
        last_update_time = std::chrono::steady_clock::now();
    }
    Informer(int socket) : socket(socket) {
        last_update_time = std::chrono::steady_clock::now();
    }

    std::string informer_id;
    SystemInformation info;
    SystemUsage usage;
    int socket;
    std::chrono::steady_clock::time_point last_update_time;

    void update_usage(uint64_t cpu, uint64_t memory, uint64_t network_upload, uint64_t network_download, uint64_t disk) {
        usage.cpu_usage = cpu;
        usage.memory_usage = memory;
        usage.network_upload = network_upload;
        usage.network_download = network_download;
        usage.disk_used_gb = disk;
    }

    bool has_timed_out() const {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - last_update_time);
        return duration.count() > INFORMER_TIMEOUT;
    }

    void update_last_time() {
        last_update_time = std::chrono::steady_clock::now();
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
    std::map<std::string, Overseer> overseers;
    std::mutex overseers_mutex;
    std::string password;

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
                case INF_INIT: // Informer Initialization
                    handle_informer_init(client_socket, buffer);
                    break;
                case INF_USAGE: // Informer System Info
                    handle_system_info(client_socket, buffer);
                    break;
                case OS_AUTH:
                    handle_overseer_auth(client_socket, buffer);
                    break;
                case OS_PONG: {
                    // Set connection_alive to true if we get a reply
                    std::string overseer_id = std::string(buffer + 1, 32);
                    if (overseers.find(overseer_id) != overseers.end()) {
                        std::lock_guard<std::mutex> lock(overseers_mutex);
                        overseers[overseer_id].connection_alive = true;
                    } else {
                        std::cout << "Received unkown overseer_id in OS_PONG: " << overseer_id << std::endl;
                    }
                    break;
                }
                default:
                    std::cout << "Unknown packet type from client.\n" << ptype;
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
        response[0] = INF_APPROVE;
        response[1] = 0b00;  // No error
        memcpy(response + 2, informer_id.c_str(), informer_id.size());
        send(client_socket, response, BUFFER_SIZE, 0);

        std::cout << "Computer Initialized" << std::endl;
        informers[informer_id].display_system_information();
        std::cout << std::endl;

        send_informer_to_overseers(informer_id);
    }

    void handle_system_info(int client_socket, char* buffer) {
        uint8_t error_bit = 0b00;
        uint8_t ack_bit = 0b01;
        std::string error_msg = "";

        uint64_t memory_usage,network_upload, network_download, disk_used, cpu_usage;
        std::string informer_id(buffer + 1, 32);
        memcpy(&cpu_usage, buffer + 33 , sizeof(uint64_t));
        memcpy(&memory_usage, buffer + 41, sizeof(uint64_t));
        memcpy(&network_upload, buffer + 49, sizeof(uint64_t));
        memcpy(&network_download, buffer + 57, sizeof(uint64_t));
        memcpy(&disk_used, buffer + 65, sizeof(uint64_t));

        {
            std::lock_guard<std::mutex> lock(informers_mutex);
            if (informers.find(informer_id) != informers.end()) {
                informers[informer_id].update_usage(cpu_usage, memory_usage, network_upload, network_download, disk_used);
                informers[informer_id].usage_to_lendian();
                informers[informer_id].update_last_time();
            } else if (informers.find(informer_id) == informers.end()) {
                std::cout << "Error: ID Not Found" << std::endl;
                std::cout << "System Informaton not updated" << std::endl;
                error_bit = 0b01;
                ack_bit = 0b00;
                error_msg = "Error: ID Not Found";
            }
        }

        char response[BUFFER_SIZE] = {0};
        response[0] = INF_ACK;
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

    void handle_overseer_auth(int client_socket, char *buffer) {
        Overseer overseer(client_socket, generate_random_id()); 
        std::string password = std::string(buffer + 1, this->password.size());

        if (password != this->password) {
            
            char response[BUFFER_SIZE] = {0};
            response[0] = OS_AUTH_INFO;
            response[1] = 0b01;
            std::string error_msg = "Error: Invalid Password";
            memcpy(response + 2, error_msg.c_str() , error_msg.size());
            send(client_socket, response, BUFFER_SIZE, 0);
            std::cout << "Overseer password:" << password << ", this-> password: " << this->password;
            std::cout << "Overseer faild to authenticate" << std::endl;
            return;
        }
    
        // Create entry in the hashmap for the connected oveerseer.
        {
            std::lock_guard<std::mutex> lock(overseers_mutex);
            overseers[overseer.overseer_id] = overseer;
        }

        // Accept Authentication
        char response[BUFFER_SIZE] = {0};
        response[0] = OS_AUTH_INFO;
        response[1] = 0b00;
        send(client_socket, response, BUFFER_SIZE, 0);

        std::cout << "New Overseer: " << overseer.overseer_id << " Authenticated" << std::endl;

        send_all_informers_to_overseer(overseer.overseer_id);

    }

    // Sends System Information of all informers to overseer.
    void send_all_informers_to_overseer(std::string overseer_id) {
        for (const auto& informer_pair: informers) {
            // Setup the packet
            char packet[BUFFER_SIZE] = {0};
            packet[0] = OS_INFORMER_INFO;
            memcpy(packet + 1, informer_pair.second.informer_id.c_str(), informer_pair.second.informer_id.size());
            memcpy(packet + 33, informer_pair.second.info.computer_name.c_str(), informer_pair.second.info.computer_name.size());
            memcpy(packet + 65, informer_pair.second.info.platform.c_str(), informer_pair.second.info.platform.size());
            memcpy(packet + 81, informer_pair.second.info.cpu_model.c_str(), informer_pair.second.info.cpu_model.size());
            
            *(packet + 113) = informer_pair.second.info.cores;

            uint16_t memory_gb = ntohs(informer_pair.second.info.memory_gb);
            uint16_t swap_gb = ntohs(informer_pair.second.info.swap_gb);
            uint64_t storage_gb = ntohll(informer_pair.second.info.storage_gb);
            
            memcpy(packet + 114, &memory_gb, sizeof(uint16_t));
            memcpy(packet + 116, &swap_gb, sizeof(uint16_t));
            memcpy(packet + 118, &storage_gb, sizeof(uint64_t));

            // Send to overseer.
            {
                std::cout << "Sending info to " << overseers[overseer_id].overseer_id << std::endl;
                std::lock_guard<std::mutex> lock(overseers_mutex);
                send(overseers[overseer_id].socket, packet, BUFFER_SIZE, 0);
            }
        }
    }

    // Sends System Information of a single informer to all overseers.
    void send_informer_to_overseers(std::string informer_id) {
       for (const auto& overseer_pair: overseers) {
            //Setup the packet 
            char packet[BUFFER_SIZE] = {0};
            packet[0] = OS_INFORMER_INFO;
            memcpy(packet + 1, informers[informer_id].informer_id.c_str(), informers[informer_id].informer_id.size());
            memcpy(packet + 33, informers[informer_id].info.computer_name.c_str(), informers[informer_id].info.computer_name.size());
            memcpy(packet + 65, informers[informer_id].info.platform.c_str(), informers[informer_id].info.platform.size());
            memcpy(packet + 81, informers[informer_id].info.cpu_model.c_str(), informers[informer_id].info.cpu_model.size());
            
            *(packet + 113) = informers[informer_id].info.cores;

            uint16_t memory_gb = ntohs(informers[informer_id].info.memory_gb);
            uint16_t swap_gb = ntohs(informers[informer_id].info.swap_gb);
            uint64_t storage_gb = ntohll(informers[informer_id].info.storage_gb);
            
            memcpy(packet + 114, &memory_gb, sizeof(uint16_t));
            memcpy(packet + 116, &swap_gb, sizeof(uint16_t));
            memcpy(packet + 118, &storage_gb, sizeof(uint64_t));

            // Send to overseer
            {
                std::lock_guard<std::mutex> lock(overseers_mutex);
                send(overseer_pair.second.socket, packet, BUFFER_SIZE, 0);
            }
        } 
    }

    // Sends system usage information to all overseers, this should be run at an interval.
    // Running it everytime we get an update from an informer would have too much of an effect on performance.
    void update_overseers() {
        for (const auto& overseer_pair: overseers) {
            for (const auto& informer_pair: informers) {
                //Setup the packet
                char packet[BUFFER_SIZE] = {0};
                packet[0] = OS_UPDATE_USG;
                memcpy(packet + 1, informer_pair.second.informer_id.c_str(), informer_pair.second.informer_id.size());
                
                uint64_t cpu_usage = ntohll(informer_pair.second.usage.cpu_usage);
                uint64_t memory_usage = ntohll(informer_pair.second.usage.memory_usage);
                uint64_t network_download = ntohll(informer_pair.second.usage.network_download);
                uint64_t network_upload = ntohll(informer_pair.second.usage.network_upload);
                uint64_t disk_used_gb = ntohll(informer_pair.second.usage.disk_used_gb);

                memcpy(packet + 33, &cpu_usage, sizeof(uint64_t));
                memcpy(packet + 41, &memory_usage, sizeof(uint64_t));
                memcpy(packet + 49, &network_download, sizeof(uint64_t));
                memcpy(packet + 57, &network_upload, sizeof(uint64_t));
                memcpy(packet + 65, &disk_used_gb, sizeof(uint64_t));

                // Send to overseer
                {
                    std::lock_guard<std::mutex> lock(overseers_mutex);
                    send(overseer_pair.second.socket, packet, BUFFER_SIZE, 0);
                }
            }
        }
    }

    std::string generate_random_id() {
        const std::string characters = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        thread_local std::random_device rd;
        thread_local std::mt19937 gen(rd());
        thread_local std::uniform_int_distribution<> dis(0, characters.size() - 1);

        std::string id;
        id.reserve(32);
        for (int i = 0; i < 32; ++i) {
            id += characters[dis(gen)];
        }

        return id;
    }

    void update_overseers_periodically() {
        while (true) {
            update_overseers();
            std::this_thread::sleep_for(std::chrono::seconds(UPDATE_OVERSEERS_INTERVAL));
        }
    }

    // Tells all overseers about the timed out informer.
    void report_informer_timeout(std::string informer_id, std::string reason) {
        for (const auto& overseer_pair: overseers) {
            char packet[BUFFER_SIZE] = {0};
            packet[0] = OS_NOTIFY_INFORMER_TIMEOUT;
            memcpy(packet + 1, informer_id.c_str(), informer_id.size());
            memcpy(packet + 33, reason.c_str(), reason.size());

            // Send timeout to overseer
            {
                std::lock_guard<std::mutex> lock(overseers_mutex);
                send(overseer_pair.second.socket, packet, BUFFER_SIZE, 0);
            }
        }
    }

    // Runs every 10 seconds, If an informer doesn't provide information for more than 10 seconds it's removed from the map.
    void cleanup_informers() {
      for (auto informer_iter = informers.begin(); informer_iter != informers.end();) {
          std::string informer_id;
          if (informer_iter->second.has_timed_out()) {
              // Lock Mutex and cleanup informer
              {
                std::lock_guard<std::mutex> lock(informers_mutex);
                if (informer_iter->second.socket) {
                    close(informer_iter->second.socket);
                }
                
                informer_id = informer_iter->second.informer_id;
                // Remove from map
                informer_iter = informers.erase(informer_iter);
              }
              std::cout << "Removed Informer ID: " << informer_id << " (Timed out)" << std::endl;
              // Notify Overseers of timed out informer
              report_informer_timeout(informer_id, "Timed out");
          } else {
              ++informer_iter;
          }
      } 
    }

    void cleanup_informers_periodically(){
        while (true) {
            cleanup_informers();
            std::this_thread::sleep_for(std::chrono::seconds(CLEANUP_INFORMERS_INTERVAL));
        }
    }

    // Checks whether overseers are still active, removes them if not.
    void cleanup_overseers() {

        // Set connection_alive to false and send a ping to all overseers.
        {
            std::lock_guard<std::mutex> lock(overseers_mutex);
            for (auto &overseer_pair: overseers) {
                overseer_pair.second.connection_alive = false;

                char packet[BUFFER_SIZE] = {0};
                packet[0] = OS_PING;
                memcpy(packet + 1, overseer_pair.first.c_str(), overseer_pair.first.size());
                send(overseer_pair.second.socket, packet, BUFFER_SIZE, 0); 
            }
        }

        // Wait for overseer's reply
        std::this_thread::sleep_for(std::chrono::seconds(OVERSEER_TIMEOUT));

        // If an overseer has replied, then handle_connection should have received it and set connection_alive for that overseer to true.
        // If it is false then we can time out the overseer.
         for (auto overseer_iter = overseers.begin(); overseer_iter != overseers.end();) {
            if (!overseer_iter->second.connection_alive) {
                {
                    std::lock_guard<std::mutex> lock(overseers_mutex);
                    if (overseer_iter->second.socket)
                        close(overseer_iter->second.socket);
                    overseer_iter = overseers.erase(overseer_iter);
                }
            } else 
                overseer_iter++;
         }
       
    }


    void cleanup_overseers_periodically() {
        while (true) {
            cleanup_overseers();
            std::this_thread::sleep_for(std::chrono::seconds(CLEANUP_OVERSEERS_INTERVAL));

        }
    }

public:
    NetMonServer(std::string password) : password(password) {};

    void run() {
        initialize_socket();
        // Sends usage information to overseers at an interval.
        //std::thread(&NetMonServer::update_overseers_periodically, this).detach();
        // Cleans up timed out informers
        //std::thread(&NetMonServer::cleanup_informers_periodically, this).detach();
        // Cleans up inactive overseers
        //std::thread(&NetMonServer::cleanup_overseers_periodically, this).detach();

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

int main(int argc, char *argv[]) {
    if (argc != 2) {
        std::cout << "Usage: ./Server <Overseer Password>" << std::endl;
        return 1;
    }

    NetMonServer server(argv[1]);
    server.run();
    return 0;
}
