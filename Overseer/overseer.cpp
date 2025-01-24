#include <iostream>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <thread>
#include <chrono>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 128

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

class Overseer {
private:
    int sock;
    std::string password;
    char buffer[BUFFER_SIZE] = {0};

    void send_authentication_request() {
        memset(buffer, 0, BUFFER_SIZE);
        buffer[0] = OS_AUTH;
        memcpy(buffer + 1, password.c_str(), password.size());

        send(sock, buffer, BUFFER_SIZE, 0);
        std::cout << "Sent authentication request to server...\n";
    }

    void handle_response() {
        while (true) {
            memset(buffer, 0, BUFFER_SIZE);
            int bytes_received = recv(sock, buffer, BUFFER_SIZE, 0);

            if (bytes_received <= 0) {
                std::cout << "Connection lost or no data received.\n";
                break;
            }

            uint8_t ptype = buffer[0];
            switch (ptype) {
                case OS_AUTH_INFO: {
                    handle_auth_info(buffer);
                    break;
                }
                case OS_INFORMER_INFO: {
                    handle_informer_info(buffer);
                    break;
                }
                case OS_UPDATE_USG: {
                    handle_update_usage(buffer);
                    break;
                }
                case OS_PING: {
                    handle_ping();
                    break;
                }
                default: {
                    std::cout << "Received unknown packet type: " << (int)ptype << "\n";
                    print_raw_data(bytes_received);
                    break;
                }
            }
        }
    }

    void handle_auth_info(const char* buffer) {
        if (buffer[1] == 0) {
            std::cout << "Authentication successful.\n";
        } else {
            std::cout << "Authentication failed: " << (buffer + 2) << "\n";
        }
    }

    void handle_informer_info(const char* buffer) {
    // Extract information from the buffer
    std::string informer_id(buffer + 1, 32);
    std::string computer_name(buffer + 33, 32);
    std::string platform(buffer + 65, 16);
    std::string cpu_model(buffer + 81, 32);
    uint8_t cores = buffer[113];
    uint16_t memory_gb = ntohs(*(uint16_t*)(buffer + 114));
    uint16_t swap_gb = ntohs(*(uint16_t*)(buffer + 116));
    uint64_t storage_gb = ntohll(*(uint64_t*)(buffer + 118));

    // Print the extracted details with proper spacing
    std::cout << "\nInformer ID: " << informer_id << "\n";
    std::cout << "Computer Name: " << computer_name << "\n";
    std::cout << "Platform: " << platform << "\n";
    std::cout << "CPU Model: " << cpu_model << "\n";
    std::cout << "Cores: " << static_cast<int>(cores) << "\n";
    std::cout << "Memory (GB): " << memory_gb << "\n";
    std::cout << "Swap (GB): " << swap_gb << "\n";
    std::cout << "Storage (GB): " << storage_gb << "\n";
}


    void handle_update_usage(const char* buffer) {
        std::string informer_id(buffer + 1, 32);
        uint64_t cpu_usage = ntohll(*(uint64_t*)(buffer + 33));
        uint64_t memory_usage = ntohll(*(uint64_t*)(buffer + 41));
        uint64_t network_download = ntohll(*(uint64_t*)(buffer + 49));
        uint64_t network_upload = ntohll(*(uint64_t*)(buffer + 57));
        uint64_t disk_used_gb = ntohll(*(uint64_t*)(buffer + 65));

        float cpu_usage_float = static_cast<float>(cpu_usage) / 100.0f;      // Convert to percentage with 2 decimal places
        float memory_usage_float = static_cast<float>(memory_usage) / 100.0f; // Convert to percentage with 2 decimal places
        float network_download_float = static_cast<float>(network_download) / 100.0f; // Convert to MB/s
        float network_upload_float = static_cast<float>(network_upload) / 100.0f; 

        std::cout << "\n";
        std::cout << "OS Update Usage Packet:\n";
        std::cout << "Informer ID: " << informer_id << "\n";
        std::cout << "CPU Usage: " << cpu_usage_float << " %" << "\n";
        std::cout << "Memory Usage: " << memory_usage_float << " %" << "\n";
        std::cout << "Network Download: " << network_download_float << " MB" << "\n";
        std::cout << "Network Upload: " << network_upload_float << " MB" << "\n";
        std::cout << "Disk Used: " << disk_used_gb << " GB" << "\n";
    }

    void handle_ping() {
        std::cout << "Received PING, sending PONG...\n";
        buffer[0] = OS_PONG;
        send(sock, buffer, BUFFER_SIZE, 0);
    }

    void print_raw_data(int length) {
        std::cout << "Raw Data (Hex): ";
        for (int i = 0; i < length; ++i) {
            std::cout << std::hex << (int)buffer[i] << " ";
        }
        std::cout << std::dec << std::endl;
    }

    uint64_t ntohll(uint64_t val) {
        return ((uint64_t)ntohl(val & 0xFFFFFFFF) << 32) | ntohl(val >> 32);
    }

public:
    Overseer(const std::string &server_ip, int port, const std::string &password)
        : password(password) {
        struct sockaddr_in server_addr;
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            std::cerr << "Error creating socket\n";
            exit(EXIT_FAILURE);
        }

        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(port);
        server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());

        if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Error connecting to server\n";
            exit(EXIT_FAILURE);
        }

        std::cout << "Connected to server at " << server_ip << ":" << port << std::endl;
    }

    void start() {
        std::thread auth_thread(&Overseer::send_authentication_request, this);
        auth_thread.join();

        std::thread listen_thread(&Overseer::handle_response, this);
        listen_thread.detach();
    }

    ~Overseer() {
        close(sock);
    }
};

int main() {
    std::string password = "abc123";
    Overseer overseer(SERVER_IP, SERVER_PORT, password);
    overseer.start();

    // Wait indefinitely (or until some exit condition is met)
    std::this_thread::sleep_for(std::chrono::hours(24));
    return 0;
}
