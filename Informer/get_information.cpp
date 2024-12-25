#ifndef INFORMATION_GRABBER_H
#define INFORMATION_GRABBER_H

#include <iostream>
#include <stdexcept>

struct SystemInformation {
    float cpu;
    float memory;
    float disk;
    float network;
    
    SystemInformation() : cpu(0), memory(0), disk(0), network(0) {}
};

class InformationGrabber {
private:
    float get_cpu_usage() {
        // TODO: Implement actual CPU monitoring
        return 20.0f;
    }

    float get_memory_usage() {
        // TODO: Implement actual memory monitoring
        return 30.0f;
    }

    float get_disk_usage() {
        // TODO: Implement actual disk monitoring
        return 40.0f;
    }

    float get_network_usage() {
        // TODO: Implement actual network monitoring
        return 50.0f;
    }

public:
    InformationGrabber() {}
    ~InformationGrabber() = default;

    int get_information(SystemInformation& info) {
        try {
            info.cpu = get_cpu_usage();
            info.memory = get_memory_usage();
            info.disk = get_disk_usage();
            info.network = get_network_usage();
            return 0;
        } catch (const std::exception& e) {
            std::cerr << "Error getting system information: " << e.what() << std::endl;
            return 1;
        }
    }
};

#endif

int main() {
    InformationGrabber grabber;
    SystemInformation info;
    
    if (grabber.get_information(info) == 0) {
        std::cout << "CPU: " << info.cpu << "%" << std::endl;
        std::cout << "Memory: " << info.memory << "%" << std::endl;
        std::cout << "Disk: " << info.disk << "%" << std::endl;
        std::cout << "Network: " << info.network << "%" << std::endl;
    }
    return 0;
}

/*
Example Usage:

int main() {
    InformationGrabber grabber;
    SystemInformation info;
    
    if (grabber.get_information(info) == 0) {
        std::cout << "CPU: " << info.cpu << "%" << std::endl;
        std::cout << "Memory: " << info.memory << "%" << std::endl;
        std::cout << "Disk: " << info.disk << "%" << std::endl;
        std::cout << "Network: " << info.network << "%" << std::endl;
    }
    return 0;
}
 */