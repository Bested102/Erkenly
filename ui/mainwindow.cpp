#include "mainwindow.h"
#include "lotoverview.h"
#include "AppController.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFont>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Campus Parking Navigator");
    resize(800, 500);

    controller = new AppController(this);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    auto *layout = new QVBoxLayout(central);
    layout->setContentsMargins(40, 40, 40, 40);
    layout->setSpacing(20);

    layout->addStretch();

    auto *titleLabel = new QLabel("Campus Parking Navigator", this);
    QFont titleFont;
    titleFont.setPointSize(20);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);

    auto *subtitleLabel = new QLabel(
        "Loads parking data from the server",
        this
        );
    subtitleLabel->setAlignment(Qt::AlignCenter);

    statusLabel = new QLabel("Status: Disconnected", this);
    statusLabel->setAlignment(Qt::AlignCenter);

    auto *viewLotsButton = new QPushButton("View Parking Lots", this);
    viewLotsButton->setMinimumHeight(45);

    auto *exitButton = new QPushButton("Exit", this);
    exitButton->setMinimumHeight(45);

    layout->addWidget(titleLabel);
    layout->addWidget(subtitleLabel);
    layout->addWidget(statusLabel);
    layout->addSpacing(20);
    layout->addWidget(viewLotsButton);
    layout->addWidget(exitButton);

    layout->addStretch();

    connect(controller, &AppController::connectionStatusChanged, this, [this](const QString &status) {
        statusLabel->setText("Status: " + status);
    });

    connect(controller, &AppController::errorOccurred, this, [this](const QString &message) {
        QMessageBox::warning(this, "Network Error", message);
    });

    connect(viewLotsButton, &QPushButton::clicked, this, [this]() {
        if (!lotOverviewWindow) {
            lotOverviewWindow = new LotOverview(controller, this);

            connect(lotOverviewWindow, &QObject::destroyed, this, [this]() {
                lotOverviewWindow = nullptr;
            });
        }

        lotOverviewWindow->show();
        lotOverviewWindow->raise();
        lotOverviewWindow->activateWindow();
        this->hide();
    });

    connect(exitButton, &QPushButton::clicked, this, [this]() {
        close();
    });

    controller->connectToServer("127.0.0.1", 5555);
}
