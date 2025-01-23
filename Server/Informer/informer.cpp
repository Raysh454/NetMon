// informer.cpp
#include "informer.h"

Informer::Informer() : socket(-1) {
    last_update_time = std::chrono::steady_clock::now();
}

Informer::Informer(int socket) : socket(socket) {
    last_update_time = std::chrono::steady_clock::now();
}

void Informer::update_usage(uint64_t cpu, uint64_t memory, uint64_t network_upload, uint64_t network_download, uint64_t disk) {
    usage.cpu_usage = cpu;
    usage.memory_usage = memory;
    usage.network_upload = network_upload;
    usage.network_download = network_download;
    usage.disk_used_gb = disk;
}

bool Informer::has_timed_out() const {
    auto now = std::chrono::steady_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(now - last_update_time);
    return duration.count() > INFORMER_TIMEOUT;
}

void Informer::update_last_time() {
    last_update_time = std::chrono::steady_clock::now();
}

void Informer::usage_to_lendian() {
    usage.cpu_usage = ntohll(usage.cpu_usage);
    usage.memory_usage = ntohll(usage.memory_usage);
    usage.network_upload = ntohll(usage.network_upload);
    usage.network_download = ntohll(usage.network_download);
    usage.disk_used_gb = ntohll(usage.disk_used_gb);
}

void Informer::sysinfo_to_lendian() {
    info.memory_gb = ntohs(info.memory_gb);
    info.swap_gb = ntohs(info.swap_gb);
    info.storage_gb = ntohll(info.storage_gb);
}

void Informer::display_system_information() {
    std::cout << "Informer id: " << informer_id << std::endl;
    std::cout << "Computer Name: " << info.computer_name << std::endl;
    std::cout << "Platform: " << info.platform << std::endl;
    std::cout << "CPU Model: " << info.cpu_model << std::endl;
    printf("Number of Cores: %d\n", info.cores);
    printf("Memory: %u\n", info.memory_gb);
    printf("Total Storage: %lu\n", info.storage_gb);
}

void Informer::display_system_usage() {
    std::cout << "Informer id: " << informer_id << std::endl;
    printf("CPU Usage: %.2f%%\n", (float)usage.cpu_usage / 100.0);
    printf("Memory Used: %.2f%%\n", (float)usage.memory_usage / 100.0);
    printf("Network Up/Down: %.2f/%.2f MB/s\n", 
           (float)usage.network_upload / 100.0, 
           (float)usage.network_download / 100.0);
    printf("Disk space used: %lu GB\n", usage.disk_used_gb);
}

