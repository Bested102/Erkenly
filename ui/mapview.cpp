#include "mapview.h"
#include "AppController.h"
#include "ParkingLot.hpp"
#include "ParkingSpot.hpp"

#include <QWidget>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QCloseEvent>
#include <QFont>
#include <QLayout>
#include <QScrollArea>
#include <QGraphicsDropShadowEffect>

#include <algorithm>
#include <vector>
#include <string>

// --- Visual Generation for Spots ---
static QPushButton* createSpotButton(const QString &spotId, bool isFree, QWidget *parent)
{
    auto *button = new QPushButton(parent);
    button->setFixedSize(140, 80); // Size of the parking space
    button->setCursor(Qt::PointingHandCursor);

    if (isFree) {
        // Looks like empty painted lines on asphalt
        button->setText(spotId + "\nFREE");
        button->setStyleSheet(R"(
            QPushButton {
                background-color: transparent;
                border: 2px solid #718096; /* Gray parking lines */
                color: #48BB78; /* Green text */
                font-weight: bold;
                font-size: 14px;
                border-radius: 4px;
            }
            QPushButton:hover {
                background-color: rgba(72, 187, 120, 0.1);
                border-color: #48BB78;
            }
        )");
    } else {
        // Looks like a car parked inside the space
        button->setText(spotId);
        button->setStyleSheet(R"(
            QPushButton {
                background-color: #FC8181; /* Soft red car */
                border: 2px solid #E53E3E;
                color: white;
                font-weight: bold;
                font-size: 14px;
                border-radius: 12px; /* Curved car roof */
                margin: 8px 15px; /* Creates the illusion of the car being smaller than the space */
            }
            QPushButton:hover {
                background-color: #E53E3E;
            }
        )");

        // Give the "car" a slight drop shadow
        auto *shadow = new QGraphicsDropShadowEffect(button);
        shadow->setBlurRadius(8);
        shadow->setColor(QColor(0, 0, 0, 80));
        shadow->setOffset(0, 3);
        button->setGraphicsEffect(shadow);
    }

    return button;
}

static void clearLayout(QLayout *layout)
{
    while (QLayoutItem *item = layout->takeAt(0)) {
        if (QWidget *widget = item->widget()) {
            widget->deleteLater();
        }
        if (QLayout *childLayout = item->layout()) {
            clearLayout(childLayout);
        }
        delete item;
    }
}

MapView::MapView(const QString &lotId,
                 const QString &lotName,
                 AppController *controller,
                 QWidget *previousWindow)
    : QMainWindow(nullptr),
    m_previousWindow(previousWindow),
    controller(controller),
    currentLotId(lotId),
    currentLotName(lotName)
{
    setWindowTitle(lotName + " Map");
    resize(950, 700);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(40, 40, 40, 30);
    mainLayout->setSpacing(20);

    // --- Headers ---
    auto *titleLabel = new QLabel(lotName + " - Live Map", this);
    titleLabel->setObjectName("titleLabel");

    auto *infoLabel = new QLabel("Click on a parking bay to manually toggle its occupancy status.", this);
    infoLabel->setObjectName("infoLabel");

    // --- The Parking Lot Scroll Area ---
    QScrollArea *scrollArea = new QScrollArea(this);
    scrollArea->setWidgetResizable(true);
    scrollArea->setObjectName("scrollArea");
    scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

    // The "Asphalt" Container
    asphaltContainer = new QWidget(scrollArea);
    asphaltContainer->setObjectName("asphaltContainer");

    spotsGrid = new QGridLayout(asphaltContainer);
    spotsGrid->setSpacing(10);
    spotsGrid->setContentsMargins(40, 40, 40, 40);
    spotsGrid->setAlignment(Qt::AlignTop | Qt::AlignHCenter); // Keep the road centered

    scrollArea->setWidget(asphaltContainer);

    // --- Controls ---
    auto *backButton = new QPushButton("← Back to Lots", this);
    backButton->setObjectName("secondaryButton");
    backButton->setMinimumHeight(45);
    backButton->setCursor(Qt::PointingHandCursor);

    auto *buttonsLayout = new QHBoxLayout();
    buttonsLayout->addWidget(backButton);
    buttonsLayout->addStretch();

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(infoLabel);
    mainLayout->addWidget(scrollArea, 1);
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
        #asphaltContainer {
            background-color: #2D3748; /* Dark gray asphalt */
            border-radius: 16px;
            border: 4px solid #4A5568; /* Concrete curb */
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

    connect(backButton, &QPushButton::clicked, this, [this]() {
        if (m_previousWindow) m_previousWindow->show();
        hide();
    });

    connect(controller, &AppController::modelChanged, this, [this]() {
        rebuildSpots();
    });

    rebuildSpots();
}

void MapView::rebuildSpots()
{
    clearLayout(spotsGrid);

    const auto &lots = controller->getModel().getLots();
    const std::string lotIdStd = currentLotId.toStdString();

    auto lotIt = lots.find(lotIdStd);
    if (lotIt == lots.end()) return;

    const ParkingLot &lot = lotIt->second;
    const auto &spotMap = lot.getSpots();

    if (spotMap.empty()) return;

    std::vector<std::string> spotIds;
    spotIds.reserve(spotMap.size());
    for (const auto &pair : spotMap) {
        spotIds.push_back(pair.first);
    }
    std::sort(spotIds.begin(), spotIds.end());

    // Setup the road width
    spotsGrid->setColumnMinimumWidth(1, 120); // Center driving lane

    for (int i = 0; i < static_cast<int>(spotIds.size()); ++i) {
        const std::string &spotIdStd = spotIds[i];
        const ParkingSpot &spot = spotMap.at(spotIdStd);

        const bool isFree = !spot.isOccupied();
        const bool newOccupied = isFree;

        const QString spotId = QString::fromStdString(spotIdStd);
        const QString currentStatus = isFree ? "Free" : "Occupied";
        const QString newStatus = newOccupied ? "Occupied" : "Free";

        QPushButton *button = createSpotButton(spotId, isFree, asphaltContainer);

        connect(button, &QPushButton::clicked, this, [this, spotId, currentStatus, newOccupied, newStatus]() {
            const auto reply = QMessageBox::question(
                this,
                "Update Parking Bay",
                "Update spot " + spotId + " to " + newStatus + "?",
                QMessageBox::Yes | QMessageBox::No
                );

            if (reply == QMessageBox::Yes) {
                controller->reportSpot(currentLotId, spotId, newOccupied);
            }
        });

        // Alternate left (col 0) and right (col 2) sides of the lane
        const bool isLeft = (i % 2 == 0);
        const int row = i / 2;
        const int col = isLeft ? 0 : 2;

        spotsGrid->addWidget(button, row, col);

        // Add the yellow dashed lane marker in the middle (col 1) for every row
        if (isLeft) {
            QWidget *laneDash = new QWidget(asphaltContainer);
            laneDash->setFixedSize(6, 40);
            laneDash->setStyleSheet("background-color: #ECC94B; border-radius: 2px;"); // Yellow road dash
            spotsGrid->addWidget(laneDash, row, 1, Qt::AlignCenter);
        }
    }
}

void MapView::closeEvent(QCloseEvent *event)
{
    if (m_previousWindow) {
        m_previousWindow->show();
    }
    QMainWindow::closeEvent(event);
}