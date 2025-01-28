#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QMessageBox>
#include <qobject.h>
#include <QRegularExpression>
#include <qlistwidget.h>
#include "ui_main.h"
#include "../LoginWindow/login_window.h"
#include "../Overseer/overseer.h"

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);

private:
    Ui::MainWindow ui;
    LoginWindow *loginWindow;
    Overseer *overseer;
    QStringList informersList;
    QListWidgetItem* currentClickedItem = nullptr;

private slots:
    void handleLoginSuccess(Overseer *overseer);
    void handleInformerUpdated(const std::string informer_id);
    void handleInformerDisconnected(const std::string informer_id);
    void onInformerClicked(QListWidgetItem *item);
    void onDisconnectClicked();
    void clearSystemInfoArea();
};

#endif
