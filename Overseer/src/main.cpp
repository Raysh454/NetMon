#include <QApplication>
#include "MainWindow/main_window.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    MainWindow mainWindow;

    return app.exec();
}

