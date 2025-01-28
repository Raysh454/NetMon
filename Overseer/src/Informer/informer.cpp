#include "informer.h"

void Informer::update_usage(uint64_t cpu, uint64_t memory, uint64_t network_upload, uint64_t network_download, uint64_t disk) {
    usage.cpu_usage = ntohll(cpu);
    usage.memory_usage = ntohll(memory);
    usage.network_upload = ntohll(network_upload);
    usage.network_download = ntohll(network_download);
    usage.disk_used_gb = ntohll(disk);
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
    printf("Network Up/Down: %.2f/%.2f MB/s\n", (float)usage.network_upload / 100.0, (float)usage.network_download / 100.0);
    printf("Disk space used: %lu GB\n", usage.disk_used_gb);
}
