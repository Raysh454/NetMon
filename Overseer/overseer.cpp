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

    void send_authentication_request() {
        char buffer[BUFFER_SIZE] = {0};
        buffer[0] = OS_AUTH;
        memcpy(buffer + 1, password.c_str(), password.size());

        send(sock, buffer, BUFFER_SIZE, 0);
        std::cout << "Sent authentication request to server...\n";
    }

    // Custom function to convert 64-bit values from network byte order to host byte order
    uint64_t ntohll(uint64_t val) {
        return ((uint64_t)ntohl(val & 0xFFFFFFFF) << 32) | ntohl(val >> 32);
    }

    void handle_response() {
        char buffer[BUFFER_SIZE];
        while (true) {
            memset(buffer, 0, BUFFER_SIZE);
            int bytes_received = recv(sock, buffer, BUFFER_SIZE, 0);

            if (bytes_received <= 0) {
                std::cout << "Connection lost.\n";
                break;
            }

            uint8_t ptype = buffer[0];
            switch (ptype) {
                case OS_AUTH_INFO: {
                    if (buffer[1] == 0) {
                        std::cout << "Authentication successful." << std::endl;
                    } else {
                        std::cout << "Authentication failed: " << buffer + 2 << std::endl;
                        return;
                    }
                    break;
                }
                case OS_INFORMER_INFO: {
                    // Handle informer system info
                    std::string informer_id(buffer + 1, 32);
                    std::string computer_name(buffer + 33, 32);
                    std::string platform(buffer + 65, 16);
                    std::string cpu_model(buffer + 81, 32);
                    uint8_t cores = buffer[113];
                    uint16_t memory_gb = ntohs(*(uint16_t*)(buffer + 114));
                    uint16_t swap_gb = ntohs(*(uint16_t*)(buffer + 116));
                    uint64_t storage_gb = ntohll(*(uint64_t*)(buffer + 118));

                    std::cout << "Received Information from Informer " << informer_id << std::endl;
                    std::cout << "Computer Name: " << computer_name << std::endl;
                    std::cout << "Platform: " << platform << std::endl;
                    std::cout << "CPU Model: " << cpu_model << std::endl;
                    std::cout << "Cores: " << (int)cores << std::endl;
                    std::cout << "Memory (GB): " << memory_gb << std::endl;
                    std::cout << "Swap (GB): " << swap_gb << std::endl;
                    std::cout << "Storage (GB): " << storage_gb << std::endl;
                    break;
                }
                case OS_PING: {
                    // Respond to a ping request from the server
                    std::cout << "Received PING, sending PONG...\n";
                    buffer[0] = OS_PONG;
                    send(sock, buffer, BUFFER_SIZE, 0);
                    break;
                }
                default:
                    std::cout << "Unknown packet type received: " << (int)ptype << std::endl;
            }
        }
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
