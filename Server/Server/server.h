#include "../include/common.h"
#include "../Overseer/overseer.h"
#include "../Informer/informer.h"

class NetMonServer {
private:
    int server_fd;
    std::map<std::string, Informer> informers;
    std::mutex informers_mutex;
    std::map<std::string, Overseer> overseers;
    std::mutex overseers_mutex;
    std::string password;

    void initialize_socket();
    void handle_client(int client_socket);
    void handle_informer_init(int client_socket, char* buffer);
    void handle_system_info(int client_socket, char* buffer);
    void handle_overseer_auth(int client_socket, char *buffer);
    void send_all_informers_to_overseer(std::string overseer_id);
    void send_informer_to_overseers(std::string informer_id);
    void update_overseers();
    std::string generate_random_id();
    void update_overseers_periodically();
    void report_informer_timeout(std::string informer_id, std::string reason);
    void cleanup_informers();
    void cleanup_informers_periodically();
    void cleanup_overseers();
    void cleanup_overseers_periodically();

public:
    NetMonServer(std::string password);
    void run();
};

