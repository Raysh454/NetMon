#include "main_window.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent) {
    ui.setupUi(this);

    loginWindow = new LoginWindow();
    loginWindow->show();

    connect(loginWindow, &LoginWindow::loginSuccessful, this, &MainWindow::handleLoginSuccess);
    connect(ui.QInformerList, &QListWidget::itemClicked, this, &MainWindow::onInformerClicked);
    connect(ui.QDisconnectButton, &QPushButton::clicked, this, &MainWindow::onDisconnectClicked);
}

void MainWindow::handleLoginSuccess(Overseer *overseer) {
    this->overseer = overseer;
    connect(overseer, &Overseer::informer_updated, this, &MainWindow::handleInformerUpdated);
    connect(overseer, &Overseer::informer_disconnected, this, &MainWindow::handleInformerDisconnected);

    // Loop through all informers in overseer and add them to the QInformerList
    for (const auto& informer_pair : overseer->informers) {
        const std::string& informer_id = informer_pair.first;
        if (!informersList.contains(QString::fromStdString(informer_id))) {
            QListWidgetItem *item = new QListWidgetItem(QString::fromStdString(informer_pair.second.info.computer_name).remove(QRegularExpression("[^\x20-\x7E]")));
            item->setData(Qt::UserRole, QString::fromStdString(informer_id));  // Store informer_id in custom data
            ui.QInformerList->addItem(item);
            informersList.append(QString::fromStdString(informer_id));
        }
    }

    this->show();
}

void MainWindow::handleInformerUpdated(const std::string informer_id) {
    QString QInformerID = QString::fromStdString(informer_id);

    if (!informersList.contains(QInformerID)) {
        QListWidgetItem *item = new QListWidgetItem(QString::fromStdString(overseer->informers[informer_id].info.computer_name).remove(QRegularExpression("[^\x20-\x7E]")));
        item->setData(Qt::UserRole, QInformerID);  // Store informer_id in custom data
        ui.QInformerList->addItem(item);
        informersList.append(QInformerID);
    }

    if (currentClickedItem != nullptr) {
        onInformerClicked(currentClickedItem);
    }
}

void MainWindow::onInformerClicked(QListWidgetItem *item) {
    currentClickedItem = item;

    // Retrieve the informer_id from the item's custom data
    QString informer_id = item->data(Qt::UserRole).toString();  
    auto informer = overseer->informers[informer_id.toStdString()];  // Access the informer from the map in Overseer

    // Create the layout for displaying system information and usage
    QWidget *systemInfoWidget = new QWidget();
    QVBoxLayout *layout = new QVBoxLayout(systemInfoWidget);

    // Create labels for system information
    QLabel *computerNameLabel = new QLabel("Computer Name: " + QString::fromStdString(informer.info.computer_name).remove(QRegularExpression("[^\x20-\x7E]")));
    QLabel *platformLabel = new QLabel("Platform: " + QString::fromStdString(informer.info.platform).remove(QRegularExpression("[^\x20-\x7E]")));
    QLabel *cpuModelLabel = new QLabel("CPU Model: " + QString::fromStdString(informer.info.cpu_model).remove(QRegularExpression("[^\x20-\x7E]")));
    QLabel *coresLabel = new QLabel("Cores: " + QString::number(informer.info.cores));
    QLabel *memoryLabel = new QLabel("Memory: " + QString::number(informer.info.memory_gb) + " GB");
    QLabel *storageLabel = new QLabel("Storage: " + QString::number(informer.info.storage_gb) + " GB");

    // Create labels for system usage
    QLabel *cpuUsageLabel = new QLabel("CPU Usage: " + QString::number(static_cast<float>(informer.usage.cpu_usage) / 100.0, 'f', 2) + "%");
    QLabel *memoryUsageLabel = new QLabel("Memory Used: " + QString::number(static_cast<float>(informer.usage.memory_usage) / 100.0, 'f', 2) + "%");
    QLabel *networkUploadLabel = new QLabel("Network Upload: " + QString::number(static_cast<float>(informer.usage.network_upload) / 100.0, 'f', 2) + " MB/s");
    QLabel *networkDownloadLabel = new QLabel("Network Download: " + QString::number(static_cast<float>(informer.usage.network_download) / 100.0, 'f', 2) + " MB/s");
    QLabel *diskUsedLabel = new QLabel("Disk Used: " + QString::number(informer.usage.disk_used_gb) + " GB");

    // Add labels to the layout
    layout->addWidget(computerNameLabel);
    layout->addWidget(platformLabel);
    layout->addWidget(cpuModelLabel);
    layout->addWidget(coresLabel);
    layout->addWidget(memoryLabel);
    layout->addWidget(storageLabel);
    layout->addWidget(cpuUsageLabel);
    layout->addWidget(memoryUsageLabel);
    layout->addWidget(networkUploadLabel);
    layout->addWidget(networkDownloadLabel);
    layout->addWidget(diskUsedLabel);

    // Set the layout to the system info widget
    systemInfoWidget->setLayout(layout);

    // Add the widget to QSystemInfo stacked widget and show it
    ui.QInformerSystemInfo->addWidget(systemInfoWidget);
    ui.QInformerSystemInfo->setCurrentWidget(systemInfoWidget);
}

void MainWindow::handleInformerDisconnected(const std::string informer_id) {
    QString QInformerID = QString::fromStdString(informer_id);
    QList<QListWidgetItem *> items = ui.QInformerList->findItems(QInformerID, Qt::MatchExactly);
    for (QListWidgetItem *item : items) {
        delete ui.QInformerList->takeItem(ui.QInformerList->row(item));
    }

    // Remove the informer ID from the tracking list
    informersList.removeOne(QInformerID);
}

void MainWindow::onDisconnectClicked() {
    // Disconnect signals before deleting overseer
    if (overseer) {
        disconnect(overseer, &Overseer::informer_updated, this, &MainWindow::handleInformerUpdated);
        disconnect(overseer, &Overseer::informer_disconnected, this, &MainWindow::handleInformerDisconnected);
    }

    // Hide current window and show login window
    this->hide();
    loginWindow->show();

    // Delete overseer object
    delete overseer;
    overseer = nullptr;
}
