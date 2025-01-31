#include "get_information.cpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/utsname.h>
#include <mntent.h>
#include <cmath>

#define BUFFER_SIZE 128
#define UPDATE_INTERVAL 5

typedef enum {
    INF_INIT = 0b00,
    INF_APPROVE = 0b01,
    INF_USAGE = 0b10,
    INF_ACK = 0b11
} PTYPE;

class Informer {
private:
    int sock;
    std::string informer_id;
    InformationGrabber info_grabber;
    std::string server_ip;
    int server_port;
    bool connected = false;

    bool initialize_socket() {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            std::cerr << "Error creating socket\n";
            return false;
        }

        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(server_port);
        server_addr.sin_addr.s_addr = inet_addr(server_ip.c_str());

        if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            std::cerr << "Connection failed\n";
            return false;
        }

        return true;
    }

    bool send_initial_info() {
        char buffer[BUFFER_SIZE] = {0};
        buffer[0] = INF_INIT;

        // Get system information
        struct utsname sys_info;
        if (uname(&sys_info) < 0) {
            std::cerr << "Failed to get system information\n";
            return false;
        }

        // Fill computer name (32 bytes)
        std::string hostname(sys_info.nodename);
        memcpy(buffer + 1, hostname.c_str(), std::min(hostname.size(), size_t(32)));

        // Fill platform (16 bytes)
        std::string platform(sys_info.sysname);
        memcpy(buffer + 33, platform.c_str(), std::min(platform.size(), size_t(16)));

        // Fill CPU model (32 bytes)
        std::string cpu = info_grabber.get_cpu_model().substr(0,32); // In production, get from /proc/cpuinfo
        memcpy(buffer + 49, cpu.c_str(), std::min(cpu.size(), size_t(32)));

        // Fill cores (1 byte)
        buffer[81] = sysconf(_SC_NPROCESSORS_ONLN);

        // Fill memory info (2 bytes each)
        uint16_t total_ram = ntohs(static_cast<uint16_t>(round(static_cast<double>(sysconf(_SC_PHYS_PAGES) * sysconf(_SC_PAGE_SIZE)) / (1024.0 * 1024.0 * 1024.0))));
        uint16_t total_swap = 0; // Get from sysinfo() in production
        memcpy(buffer + 82, &total_ram, sizeof(uint16_t));
        memcpy(buffer + 84, &total_swap, sizeof(uint16_t));

        // Fill storage (8 bytes) - get total storage across all partitions
        uint64_t total_storage = 0;
        FILE* mtab = setmntent("/etc/mtab", "r");
        if (mtab) {
            struct mntent* entry;
            while ((entry = getmntent(mtab)) != nullptr) {
                // Skip virtual filesystems
                if (std::string(entry->mnt_type).find("tmpfs") != std::string::npos ||
                    std::string(entry->mnt_type).find("devtmpfs") != std::string::npos ||
                    std::string(entry->mnt_type).find("proc") != std::string::npos ||
                    std::string(entry->mnt_type).find("sysfs") != std::string::npos ||
                    std::string(entry->mnt_type).find("devpts") != std::string::npos) {
                    continue;
                }

                struct statvfs fs_info;
                if (statvfs(entry->mnt_dir, &fs_info) == 0) {
                    total_storage += (uint64_t)fs_info.f_blocks * fs_info.f_frsize;
                }
            }
            endmntent(mtab);
            // Convert to GB
            total_storage = total_storage / (1024*1024*1024);
        }

        total_storage = htonll(total_storage);  // Convert to network byte order
        memcpy(buffer + 86, &total_storage, sizeof(uint64_t));

        if (send(sock, buffer, BUFFER_SIZE, 0) < 0) {
            std::cerr << "Failed to send initial info\n";
            return false;
        }

        // Wait for server approval
        memset(buffer, 0, BUFFER_SIZE);
        if (recv(sock, buffer, BUFFER_SIZE, 0) <= 0) {
            std::cerr << "Failed to receive server response\n";
            return false;
        }

        if (buffer[0] != INF_APPROVE || buffer[1] != 0) {
            std::cerr << "Server rejected initialization\n";
            return false;
        }

        informer_id = std::string(buffer + 2, 32);
        std::cout << "Successfully registered with ID: " << informer_id << std::endl;
        return true;
    }

    uint64_t htonll(uint64_t value) {
        // Convert host byte order to network byte order for 64-bit integers
        return ((uint64_t)htonl(value & 0xFFFFFFFF) << 32) | htonl(value >> 32);
    }

    void send_system_usage() {
        SystemInformation sys_info;
        while (connected) {
            if (info_grabber.get_information(sys_info) == 0) {
                char buffer[BUFFER_SIZE] = {0};
                buffer[0] = INF_USAGE;
                
                // Copy informer_id
                memcpy(buffer + 1, informer_id.c_str(), 32);

                // Convert values to network byte order and copy
                uint64_t cpu = htonll((uint64_t)(sys_info.cpu_usage * 100));  // Convert percentage to fixed point
                uint64_t mem = htonll((uint64_t)(sys_info.memory * 100));  // Convert percentage to fixed point
                uint64_t net_down = htonll((uint64_t)(sys_info.network_download_mbs * 100));
                uint64_t net_up = htonll((uint64_t)(sys_info.network_upload_mbs * 100));
                uint64_t disk = htonll((uint64_t)sys_info.disk_gb);

                memcpy(buffer + 33, &cpu, sizeof(uint64_t));
                memcpy(buffer + 41, &mem, sizeof(uint64_t));
                memcpy(buffer + 49, &net_down, sizeof(uint64_t));
                memcpy(buffer + 57, &net_up, sizeof(uint64_t));
                memcpy(buffer + 65, &disk, sizeof(uint64_t));

                if (send(sock, buffer, BUFFER_SIZE, 0) < 0) {
                    std::cerr << "Failed to send system usage\n";
                    connected = false;
                    break;
                }

                // Wait for acknowledgment
                memset(buffer, 0, BUFFER_SIZE);
                if (recv(sock, buffer, BUFFER_SIZE, 0) <= 0 || buffer[0] != INF_ACK) {
                    std::cerr << "Failed to receive acknowledgment\n";
                    connected = false;
                    break;
                }
            }

            std::this_thread::sleep_for(std::chrono::seconds(UPDATE_INTERVAL));
        }
    }

public:
    Informer(const std::string& ip, int port) : server_ip(ip), server_port(port) {}

    bool start() {
        if (!initialize_socket()) {
            return false;
        }

        if (!send_initial_info()) {
            close(sock);
            return false;
        }

        connected = true;
        std::thread(&Informer::send_system_usage, this).detach();
        return true;
    }

    ~Informer() {
        if (sock >= 0) {
            close(sock);
        }
    }
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        std::cout << "Usage: ./informer <server_ip> <server_port>" << std::endl;
        return 1;
    }

    Informer informer(argv[1], std::stoi(argv[2]));
    if (!informer.start()) {
        std::cerr << "Failed to start informer\n";
        return 1;
    }

    while(true) {
        std::this_thread::sleep_for(std::chrono::hours(24));
    }

    return 0;
}
