#ifndef INFORMER_H
#define INFORMER_H

#include "../Common/common.h"
#include <iostream>

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

    std::string informer_id;
    SystemInformation info;
    SystemUsage usage;
    int socket;

    void update_usage(uint64_t cpu, uint64_t memory, uint64_t network_upload, uint64_t network_download, uint64_t disk);
    void display_system_information();
    void display_system_usage();
};

#endif
