#include "../include/common.h"
#include "../Utils/utils.h"

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
    std::chrono::steady_clock::time_point last_update_time;

    Informer();
    Informer(int socket);

    void update_usage(uint64_t cpu, uint64_t memory, uint64_t network_upload, uint64_t network_download, uint64_t disk);
    bool has_timed_out() const;
    void update_last_time();
    void usage_to_lendian();
    void sysinfo_to_lendian();
    void display_system_information();
    void display_system_usage();
};
