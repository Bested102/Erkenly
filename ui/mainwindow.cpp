#include "mainwindow.h"
#include "lotoverview.h"
#include "AppController.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFont>
#include <QMessageBox>
#include <QGraphicsDropShadowEffect>
#include <QFrame>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Campus Parking Navigator");
    resize(950, 650); // Matches the size of the other windows better

    controller = new AppController(this);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0); // Background takes the whole window
    mainLayout->setAlignment(Qt::AlignCenter);

    // --- Create a Floating Central Card ---
    QFrame *cardFrame = new QFrame(this);
    cardFrame->setObjectName("cardFrame");
    cardFrame->setFixedSize(500, 380);

    // Card Drop Shadow
    auto *shadow = new QGraphicsDropShadowEffect(cardFrame);
    shadow->setBlurRadius(25);
    shadow->setColor(QColor(0, 0, 0, 15));
    shadow->setOffset(0, 8);
    cardFrame->setGraphicsEffect(shadow);

    auto *cardLayout = new QVBoxLayout(cardFrame);
    cardLayout->setContentsMargins(45, 45, 45, 45);
    cardLayout->setSpacing(15);

    // --- Headers ---
    auto *titleLabel = new QLabel("Campus Parking", this);
    titleLabel->setObjectName("titleLabel");
    titleLabel->setAlignment(Qt::AlignCenter);

    auto *subtitleLabel = new QLabel("Live Server Connection", this);
    subtitleLabel->setObjectName("subtitleLabel");
    subtitleLabel->setAlignment(Qt::AlignCenter);

    // --- Status Label (Pill Badge) ---
    statusLabel = new QLabel("● Status: Disconnected", this);
    statusLabel->setObjectName("statusLabel");
    statusLabel->setAlignment(Qt::AlignCenter);
    statusLabel->setFixedHeight(35);

    // Wrap the status label in a horizontal layout to keep it centered and small
    auto *statusLayout = new QHBoxLayout();
    statusLayout->addStretch();
    statusLayout->addWidget(statusLabel);
    statusLayout->addStretch();

    // --- Buttons ---
    auto *viewLotsButton = new QPushButton("View Parking Lots", this);
    viewLotsButton->setObjectName("primaryButton");
    viewLotsButton->setMinimumHeight(50);
    viewLotsButton->setCursor(Qt::PointingHandCursor);

    auto *exitButton = new QPushButton("Exit Application", this);
    exitButton->setObjectName("secondaryButton");
    exitButton->setMinimumHeight(50);
    exitButton->setCursor(Qt::PointingHandCursor);

    cardLayout->addStretch();
    cardLayout->addWidget(titleLabel);
    cardLayout->addWidget(subtitleLabel);
    cardLayout->addSpacing(5);
    cardLayout->addLayout(statusLayout);
    cardLayout->addSpacing(25);
    cardLayout->addWidget(viewLotsButton);
    cardLayout->addWidget(exitButton);
    cardLayout->addStretch();

    mainLayout->addWidget(cardFrame);

    // --- Master Stylesheet ---
    this->setStyleSheet(R"(
        QMainWindow {
            background-color: #F4F7F6;
        }
        #cardFrame {
            background-color: #FFFFFF;
            border-radius: 16px;
        }
        #titleLabel {
            color: #1a202c;
            font-size: 32px;
            font-weight: 900;
            font-family: 'Segoe UI', Arial, sans-serif;
            letter-spacing: -1px;
        }
        #subtitleLabel {
            color: #718096;
            font-size: 16px;
            font-family: 'Segoe UI', Arial, sans-serif;
        }
        /* Default Disconnected Style for Status Pill */
        #statusLabel {
            color: #E53E3E;
            font-size: 14px;
            font-weight: bold;
            background-color: #FFF5F5;
            border-radius: 17px;
            padding: 0px 18px;
        }
        #primaryButton {
            background-color: #3182CE;
            color: white;
            border: none;
            border-radius: 8px;
            font-weight: bold;
            font-size: 15px;
        }
        #primaryButton:hover {
            background-color: #2B6CB0;
        }
        #primaryButton:pressed {
            background-color: #2C5282;
        }
        #secondaryButton {
            background-color: #ffffff;
            color: #4a5568;
            border: 1px solid #cbd5e0;
            border-radius: 8px;
            font-weight: bold;
            font-size: 15px;
        }
        #secondaryButton:hover {
            background-color: #f7fafc;
            border: 1px solid #a0aec0;
        }
    )");

    // --- Connections ---
    connect(controller, &AppController::connectionStatusChanged, this, [this](const QString &status) {
        statusLabel->setText("● Status: " + status);

        // Dynamically change the status pill color based on connection state
        if (status.contains("Connected", Qt::CaseInsensitive) && !status.contains("Disconnected", Qt::CaseInsensitive)) {
            statusLabel->setStyleSheet(R"(
                #statusLabel {
                    color: #38A169; /* Green */
                    background-color: #F0FFF4;
                    border-radius: 17px;
                    padding: 0px 18px;
                }
            )");
        } else {
            statusLabel->setStyleSheet(R"(
                #statusLabel {
                    color: #E53E3E; /* Red */
                    background-color: #FFF5F5;
                    border-radius: 17px;
                    padding: 0px 18px;
                }
            )");
        }
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