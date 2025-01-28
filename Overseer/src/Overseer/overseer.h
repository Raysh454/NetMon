#ifndef OVERSEER_H
#define OVERSEER_H

#include "../Common/common.h"
#include "../Informer/informer.h"
#include <qobject.h>
#include <qtmetamacros.h>
#include <thread>
#include <chrono>
#include <map>

class Overseer : public QObject {
    Q_OBJECT
private:
    std::string server_ip;
    int server_port;
    std::string server_password;
    int sock;
    std::thread listen_thread;
    bool authenticated = false;
    bool stop_listening = false;

    void send_authentication_request();
    void handle_response();
    void handle_auth_info(const char* buffer);
    void handle_informer_info(const char* buffer);
    void handle_update_usage(char* buffer);
    void handle_ping(char* buffer);
    void handle_informer_timeout(char* buffer);

signals:
    void informer_updated(const std::string informer_id);
    void informer_disconnected(const std::string informer_id);

public:
    Overseer();
    ~Overseer();

    std::map<std::string, Informer> informers;

    bool connect_to_server();
    bool authenticate_to_server();

    void set_server_ip(std::string server_ip);
    void set_server_port(int server_port);
    void set_server_password(std::string server_password);
};

#endif
