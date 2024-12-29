#include <iostream>
#include <thread>
#include <cstring>
#include <arpa/inet.h>
#include <unistd.h>
#include <mutex>

#define PORT 8080
#define BUFFER_SIZE 128
#define SERVER_ADDRESS "127.0.0.1" 
#define PASSWORD "fixed_password" 

class Overseer {
private:
    int socket_fd;
    std::mutex data_mutex;
    bool is_authenticated;
    std::string informer_id;

    void connect_to_server() {
        sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(PORT);
        inet_pton(AF_INET, SERVER_ADDRESS, &server_addr.sin_addr  );

        socket_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (socket_fd < 0) {
            perror("Socket creation failed");
            exit(EXIT_FAILURE);
          }

        if (connect(socket_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
            perror("Connection failed");
            exit(EXIT_FAILURE);
         }
        std::cout << "Connected to server at " << SERVER_ADDRESS << ":" << PORT << std::endl;

    }

    void authenticate() {
        char buffer[BUFFER_SIZE] = {0};
        buffer[0] = 0x00; 
        memcpy(buffer + 1, PASSWORD, strlen(PASSWORD)); 

        
        send(socket_fd, buffer, BUFFER_SIZE, 0);

        
        int bytes_received = recv(socket_fd, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            std::cout << "Failed to authenticate. Reconnecting...\n";
            close(socket_fd);
            connect_to_server();
            authenticate();
        } else {
            uint8_t ptype = buffer[0];
            if (ptype == 0x01 && buffer[1] == 0x00) { 
                std::cout << "Authentication successful\n";
                is_authenticated = true;
                informer_id = std::string(buffer + 2, 32); 
            } else {
                std::cout << "Authentication failed\n";
                is_authenticated = false;
            }
        }
    }

    void request_system_info() {
        if (is_authenticated) {
            char buffer[BUFFER_SIZE] = {0};
            buffer[0] = 0x10; 
            memcpy(buffer + 1, informer_id.c_str(), informer_id.size());

          
            send(socket_fd, buffer, BUFFER_SIZE, 0);

          
            int bytes_received = recv(socket_fd, buffer, BUFFER_SIZE, 0);
            if (bytes_received > 0) {
                uint8_t ptype = buffer[0];
                if (ptype == 0x11) { // Response from server
                    uint8_t error_code = buffer[1];
                    uint8_t acknowledge = buffer[2];

                    if (error_code == 0x00 && acknowledge == 0x01) {
                        // Successfully received system information
                        std::cout << "System info received\n";
                        // Process and display system info here...
                    } else {
                        std::cout << "Error receiving system info\n";
                    }
                }
            }
        }
    }

    void handle_connection() {
        while (true) {
            if (!is_authenticated) {
                authenticate();
            }
            request_system_info();
            std::this_thread::sleep_for(std::chrono::seconds(5)); 
        }
    }

public:
    Overseer() : socket_fd(-1), is_authenticated(false) {}

    void run() {
        connect_to_server();
        std::thread connection_thread(&Overseer::handle_connection, this);
        connection_thread.join();
    }

    ~Overseer() {
        if (socket_fd != -1) {
            close(socket_fd);
        }
    }
};

int main() {
    Overseer overseer;
    overseer.run(); //the main function
    return 0;
}
