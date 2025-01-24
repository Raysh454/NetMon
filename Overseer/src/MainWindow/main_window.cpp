#include "main_window.h"
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    loginWindow = new LoginWindow(this);  // Create the login window as a child

    loginWindow->show();

    connect(loginWindow, &LoginWindow::loginSuccessful, this, &MainWindow::handleLoginSuccess);
}

void MainWindow::handleLoginSuccess() {
    loginWindow->close();

    QMessageBox::information(this, "Welcome", "Login successful! Loading the main application...");
    setWindowTitle("Main Application");
    resize(800, 600);  // Resize the main window
}

