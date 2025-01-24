#include "login_window.h"
#include <QMessageBox>

LoginWindow::LoginWindow(QWidget *parent) : QWidget(parent) {
    ui.setupUi(this);  // Setup the UI (this will link the form to the window)

    connect(ui.QConnectButton, &QPushButton::clicked, this, &LoginWindow::onConnectButtonClicked);
}

void LoginWindow::onConnectButtonClicked() {
    QString IP = ui.QServerIPField->text();
    QString port = ui.QServerPortField->text();
    QString password = ui.QServerPasswordField->text();

    emit loginSuccessful();
}
