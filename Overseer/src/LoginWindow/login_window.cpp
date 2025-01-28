#include "login_window.h"

LoginWindow::LoginWindow(QWidget *parent) : QWidget(parent) {
    ui.setupUi(this);
    connect(ui.QConnectButton, &QPushButton::clicked, this, &LoginWindow::onConnectButtonClicked);
}

void LoginWindow::onConnectButtonClicked() {
    Overseer* overseer = new Overseer();

    overseer->set_server_ip(ui.QServerIPField->text().toStdString());
    overseer->set_server_port(ui.QServerPortField->text().toInt());
    overseer->set_server_password(ui.QServerPasswordField->text().toStdString());

    if (!overseer->connect_to_server()) {
        QMessageBox::critical(this, "Error", "Failed to connect to server");
        delete overseer;
        return;
    }

    if (!overseer->authenticate_to_server()) {
        QMessageBox::critical(this, "Error", "Failed to authenticate, check password.");
        delete overseer;
        return;
    }

    emit loginSuccessful(overseer);
    this->close();
}
