#ifndef INFORMATION_GRABBER_H
#define INFORMATION_GRABBER_H

#include <iostream>
#include <stdexcept>
#include <fstream>
#include <string>
#include <sstream>
#include <chrono>
#include <sys/statvfs.h>
#include <unistd.h>
#include <mntent.h>

struct SystemInformation {
    std::string cpu_model;
    float cpu_usage;
    float memory;
    float disk_gb;
    float network_download_mbs;
    float network_upload_mbs;
    
    SystemInformation() : cpu_usage(0), memory(0), disk_gb(0), 
                         network_download_mbs(0), network_upload_mbs(0) {}
};

class InformationGrabber {
private:
    struct CPUData {
        unsigned long long user;
        unsigned long long nice;
        unsigned long long system;
        unsigned long long idle;
        unsigned long long iowait;
        unsigned long long irq;
        unsigned long long softirq;
        unsigned long long steal;
    };

    float get_cpu_usage() {
        static CPUData prev_cpu = {0, 0, 0, 0, 0, 0, 0, 0};
        CPUData curr_cpu = {0, 0, 0, 0, 0, 0, 0, 0};
        
        std::ifstream stat_file("/proc/stat");
        std::string line;
        std::getline(stat_file, line);
        
        if (line.substr(0, 3) != "cpu") {
            throw std::runtime_error("Failed to read CPU stats");
        }

        std::istringstream ss(line);
        std::string cpu;
        ss >> cpu 
           >> curr_cpu.user >> curr_cpu.nice 
           >> curr_cpu.system >> curr_cpu.idle 
           >> curr_cpu.iowait >> curr_cpu.irq 
           >> curr_cpu.softirq >> curr_cpu.steal;

        unsigned long long prev_idle = prev_cpu.idle + prev_cpu.iowait;
        unsigned long long curr_idle = curr_cpu.idle + curr_cpu.iowait;

        unsigned long long prev_total = prev_idle + 
            prev_cpu.user + prev_cpu.nice + prev_cpu.system + 
            prev_cpu.irq + prev_cpu.softirq + prev_cpu.steal;
        unsigned long long curr_total = curr_idle + 
            curr_cpu.user + curr_cpu.nice + curr_cpu.system + 
            curr_cpu.irq + curr_cpu.softirq + curr_cpu.steal;

        prev_cpu = curr_cpu;

        if (curr_total - prev_total == 0) return 0.0f;
        return 100.0f * (1.0f - (float)(curr_idle - prev_idle) / (float)(curr_total - prev_total));
    }

    float get_memory_usage() {
        std::ifstream meminfo("/proc/meminfo");
        std::string line;
        unsigned long total = 0, available = 0;

        while (std::getline(meminfo, line)) {
            if (line.find("MemTotal:") != std::string::npos) {
                std::istringstream iss(line);
                std::string key;
                iss >> key >> total;
            }
            if (line.find("MemAvailable:") != std::string::npos) {
                std::istringstream iss(line);
                std::string key;
                iss >> key >> available;
            }
        }

        if (total == 0) return 0.0f;
        return 100.0f * (1.0f - (float)available / (float)total);
    }

    float get_disk_usage() {
        float total_gb_used = 0;
        FILE* mtab = setmntent("/etc/mtab", "r");
        if (mtab == nullptr) {
            throw std::runtime_error("Failed to open /etc/mtab");
        }

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

            struct statvfs fs_stats;
            if (statvfs(entry->mnt_dir, &fs_stats) == 0) {
                uint64_t total = fs_stats.f_blocks * fs_stats.f_frsize;
                uint64_t free = fs_stats.f_bfree * fs_stats.f_frsize;
                uint64_t used = total - free;
                total_gb_used += static_cast<float>(used) / (1024 * 1024 * 1024);
            }
        }
        endmntent(mtab);
        return total_gb_used;
    }

    struct NetworkStats {
        unsigned long rx_bytes;
        unsigned long tx_bytes;
    };

    NetworkStats prev_stats{0, 0};
    std::chrono::steady_clock::time_point prev_time = std::chrono::steady_clock::now();

    void get_network_usage(float& download_mbps, float& upload_mbps) {
        NetworkStats curr_stats{0, 0};
        auto curr_time = std::chrono::steady_clock::now();
        
        std::ifstream netdev("/proc/net/dev");
        std::string line;
        
        // Skip header lines
        std::getline(netdev, line);
        std::getline(netdev, line);

        while (std::getline(netdev, line)) {
            std::istringstream iss(line);
            std::string iface;
            unsigned long rx_bytes, tx_bytes;
            
            iss >> iface;
            if (iface.find("lo:") != std::string::npos) continue;
            
            iss >> rx_bytes;
            for (int i = 0; i < 7; ++i) { 
                unsigned long dummy; 
                iss >> dummy; 
            }
            iss >> tx_bytes;
            
            curr_stats.rx_bytes += rx_bytes;
            curr_stats.tx_bytes += tx_bytes;
        }

        auto duration = std::chrono::duration_cast<std::chrono::seconds>(curr_time - prev_time).count();
        if (duration > 0) {
            download_mbps = static_cast<float>(curr_stats.rx_bytes - prev_stats.rx_bytes) 
                          / (1024 * 1024 * duration);
            upload_mbps = static_cast<float>(curr_stats.tx_bytes - prev_stats.tx_bytes) 
                         / (1024 * 1024 * duration);
        }

        prev_stats = curr_stats;
        prev_time = curr_time;
    }

public:
    InformationGrabber() {}
    ~InformationGrabber() = default;

    int get_information(SystemInformation& info) {
        try {
            info.cpu_model = get_cpu_model();
            info.cpu_usage = get_cpu_usage();
            info.memory = get_memory_usage();
            info.disk_gb = get_disk_usage();
            get_network_usage(info.network_download_mbs, info.network_upload_mbs);
            return 0;
        } catch (const std::exception& e) {
            std::cerr << "Error getting system information: " << e.what() << std::endl;
            return 1;
        }
    }
    std::string get_cpu_model() {
        std::ifstream cpuinfo("/proc/cpuinfo");
        std::string line;
        
        if (!cpuinfo.is_open()) {
            return "Unable to open /proc/cpuinfo";
        }

        while (std::getline(cpuinfo, line)) {
            if (line.find("model name") != std::string::npos) {
                return line.substr(line.find(":") + 2);  // Extract value after ': '
            }
        }

        return "CPU model name not found";
    }
};

#endif
