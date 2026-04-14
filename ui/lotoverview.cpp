#include "lotoverview.h"
#include "mapview.h"
#include "AppController.h"
#include "ParkingLot.hpp"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QProgressBar>
#include <QGraphicsDropShadowEffect>
#include <QCloseEvent>
#include <QFont>
#include <QCursor>

LotOverview::LotOverview(AppController *appController, QWidget *previousWindow)
    : QMainWindow(nullptr),
    m_previousWindow(previousWindow),
    controller(appController)
{
    setWindowTitle("Parking Lot Dashboard");
    resize(1000, 650); // Wider to accommodate the card grid

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(40, 40, 40, 30);
    mainLayout->setSpacing(25);

    // --- Header Section ---
    auto *titleLabel = new QLabel("Parking Dashboard", this);
    titleLabel->setObjectName("titleLabel");

    auto *infoLabel = new QLabel("Select a parking lot below to view live availability and maps.", this);
    infoLabel->setObjectName("infoLabel");

    // --- Grid Area (The core UI change) ---
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setObjectName("scrollArea");
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    QWidget *gridContainer = new QWidget(scrollArea);
    gridContainer->setObjectName("gridContainer");

    gridLayout = new QGridLayout(gridContainer);
    gridLayout->setSpacing(20);
    gridLayout->setContentsMargins(10, 10, 10, 10);
    gridLayout->setAlignment(Qt::AlignTop);

    scrollArea->setWidget(gridContainer);

    // --- Bottom Controls ---
    auto *buttonsLayout = new QHBoxLayout();
    auto *backButton = new QPushButton("← Back to Menu", this);
    backButton->setObjectName("secondaryButton");
    backButton->setMinimumHeight(45);
    backButton->setCursor(Qt::PointingHandCursor);

    buttonsLayout->addWidget(backButton);
    buttonsLayout->addStretch(); // Pushes back button to the left

    // --- Build Layout ---
    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(infoLabel);
    mainLayout->addWidget(scrollArea, 1); // 1 = stretch factor
    mainLayout->addLayout(buttonsLayout);

    // --- Master Stylesheet ---
    this->setStyleSheet(R"(
        QMainWindow {
            background-color: #F4F7F6;
        }
        #titleLabel {
            color: #1a202c;
            font-size: 28px;
            font-weight: 800;
            font-family: 'Segoe UI', Arial, sans-serif;
            letter-spacing: -0.5px;
        }
        #infoLabel {
            color: #718096;
            font-size: 15px;
            font-family: 'Segoe UI', Arial, sans-serif;
        }
        #scrollArea {
            background: transparent;
            border: none;
        }
        #gridContainer {
            background: transparent;
        }
        QPushButton#lotCard {
            margin-top: 2px;
            background-color: #FFFFFF;
            border-radius: 12px;
            border: 1px solid #E2E8F0;
            text-align: left;
        }
        QPushButton#lotCard:hover {
            border: 2px solid #3182CE;
            background-color: #F8FAFC;
        }
        QLabel#cardTitle {
            font-size: 18px;
            font-weight: bold;
            color: #2D3748;
        }
        QLabel#cardStats {
            font-size: 14px;
            color: #4A5568;
            font-weight: 500;
        }
        QProgressBar {
            border: none;
            background-color: #EDF2F7;
            border-radius: 4px;
            text-align: right;
            color: transparent; /* Hides default text */
        }
        #secondaryButton {
            background-color: #ffffff;
            color: #4a5568;
            border: 1px solid #cbd5e0;
            border-radius: 6px;
            font-weight: bold;
            font-size: 14px;
            padding: 0px 20px;
        }
        #secondaryButton:hover {
            background-color: #f7fafc;
            border: 1px solid #a0aec0;
        }
    )");

    // --- Connections ---
    connect(backButton, &QPushButton::clicked, this, [this]() {
        if (m_previousWindow) m_previousWindow->show();
        hide();
    });

    connect(controller, &AppController::modelChanged, this, [this]() {
        refreshGrid();
    });

    refreshGrid();
}

void LotOverview::refreshGrid()
{
    // 1. Clear existing cards from the grid layout
    QLayoutItem *child;
    while ((child = gridLayout->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }

    int row = 0;
    int col = 0;
    const int maxColumns = 3; // Number of cards per row

    // 2. Build a custom card for each lot
    for (const auto &pair : controller->getModel().getLots()) {
        const ParkingLot &lot = pair.second;

        QString lotId = QString::fromStdString(lot.getId());
        QString lotName = QString::fromStdString(lot.getName());
        double occupancy = lot.getOccupancyPercent();
        int freeSpots = lot.getFreeSpots();
        int totalSpots = lot.getTotalSpots();

        // The card acts as a button so it receives hover and click events naturally
        QPushButton *card = new QPushButton(this);
        card->setObjectName("lotCard");
        card->setFixedSize(280, 150); // Uniform card sizes
        card->setCursor(Qt::PointingHandCursor);

        // Card Drop Shadow
        auto *shadow = new QGraphicsDropShadowEffect(card);
        shadow->setBlurRadius(15);
        shadow->setColor(QColor(0, 0, 0, 15));
        shadow->setOffset(0, 5);
        card->setGraphicsEffect(shadow);

        // Layout inside the Card
        QVBoxLayout *cardLayout = new QVBoxLayout(card);
        cardLayout->setContentsMargins(20, 20, 20, 20);
        cardLayout->setSpacing(8);

        QLabel *titleLabel = new QLabel(lotName, card);
        titleLabel->setObjectName("cardTitle");
        titleLabel->setAttribute(Qt::WA_TransparentForMouseEvents); // Let clicks pass to the button

        QLabel *statsLabel = new QLabel(QString("%1 / %2 Spots Free").arg(freeSpots).arg(totalSpots), card);
        statsLabel->setObjectName("cardStats");
        statsLabel->setAttribute(Qt::WA_TransparentForMouseEvents);

        QProgressBar *progressBar = new QProgressBar(card);
        progressBar->setFixedHeight(8);
        progressBar->setMinimum(0);
        progressBar->setMaximum(100);
        progressBar->setValue(static_cast<int>(occupancy));
        progressBar->setAttribute(Qt::WA_TransparentForMouseEvents);

        // Dynamic styling for the progress bar based on capacity
        QString chunkColor;
        if (occupancy >= 85.0) {
            chunkColor = "#E53E3E"; // Red for full
        } else if (occupancy >= 50.0) {
            chunkColor = "#DD6B20"; // Orange for medium
        } else {
            chunkColor = "#48BB78"; // Green for empty
        }

        progressBar->setStyleSheet(QString(
                                       "QProgressBar::chunk { background-color: %1; border-radius: 4px; }"
                                       ).arg(chunkColor));

        cardLayout->addWidget(titleLabel);
        cardLayout->addStretch();
        cardLayout->addWidget(statsLabel);
        cardLayout->addWidget(progressBar);

        // Handle Map Opening logic when the card is clicked
        connect(card, &QPushButton::clicked, this, [this, lotId, lotName]() {
            if (mapViewWindow) {
                mapViewWindow->deleteLater();
                mapViewWindow = nullptr;
            }

            mapViewWindow = new MapView(lotId, lotName, this->controller, this);

            connect(mapViewWindow, &QObject::destroyed, this, [this]() {
                mapViewWindow = nullptr;
            });

            mapViewWindow->show();
            mapViewWindow->raise();
            mapViewWindow->activateWindow();
            hide();
        });

        // Add to grid and calculate next row/col
        gridLayout->addWidget(card, row, col);
        col++;
        if (col >= maxColumns) {
            col = 0;
            row++;
        }
    }
}

void LotOverview::closeEvent(QCloseEvent *event)
{
    if (m_previousWindow) {
        m_previousWindow->show();
    }
    QMainWindow::closeEvent(event);
}