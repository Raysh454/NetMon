#ifndef LOGIN_H
#define LOGIN_H

#include <QWidget>
#include <qtmetamacros.h>
#include "ui_login.h"

class LoginWindow : public QWidget {
    Q_OBJECT

public:
    explicit LoginWindow(QWidget *parent = nullptr);

signals:
    void loginSuccessful();

private slots:
    void onConnectButtonClicked();

private:
    Ui::LoginForm ui;
};

#endif
