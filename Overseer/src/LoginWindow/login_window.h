#ifndef LOGIN_H
#define LOGIN_H

#include <QWidget>
#include <qtmetamacros.h>
#include <QMessageBox>
#include <qmessagebox.h>
#include "ui_login.h"
#include "../Overseer/overseer.h"

class LoginWindow : public QWidget {
    Q_OBJECT

public:
    explicit LoginWindow(QWidget *parent = nullptr);

signals:
    void loginSuccessful(Overseer *overseer);

private slots:
    void onConnectButtonClicked();

private:
    Ui::LoginForm ui;
};

#endif
