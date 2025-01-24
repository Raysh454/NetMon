#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include "ui_main.h"
#include "../LoginWindow/login_window.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private:
    LoginWindow *loginWindow;

private slots:
    void handleLoginSuccess();
};

#endif
