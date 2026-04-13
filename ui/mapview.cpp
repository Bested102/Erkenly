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

#include <algorithm>
#include <vector>
#include <string>

static QPushButton* createSpotButton(const QString &spotText, bool isFree, QWidget *parent)
{
    auto *button = new QPushButton(spotText, parent);
    button->setMinimumSize(110, 75);

    if (isFree) {
        button->setStyleSheet("QPushButton { background-color: #7CFC98; font-weight: bold; }");
    } else {
        button->setStyleSheet("QPushButton { background-color: #FF7F7F; font-weight: bold; }");
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
    resize(900, 600);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(15);

    auto *titleLabel = new QLabel(lotName + " - Parking Map", this);
    QFont titleFont;
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    auto *legendLabel = new QLabel("Green = Free   |   Red = Occupied", this);
    auto *infoLabel = new QLabel("Click a spot to toggle and report its status to the server.", this);

    spotsGrid = new QGridLayout();
    spotsGrid->setSpacing(12);

    auto *backButton = new QPushButton("Back to Lots", this);
    backButton->setMinimumHeight(40);

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(legendLabel);
    mainLayout->addWidget(infoLabel);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(spotsGrid);
    mainLayout->addSpacing(15);
    mainLayout->addWidget(backButton);

    connect(backButton, &QPushButton::clicked, this, [this]() {
        if (m_previousWindow) {
            m_previousWindow->show();
        }
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
    if (lotIt == lots.end()) {
        auto *label = new QLabel("Lot data not found in model.", this);
        spotsGrid->addWidget(label, 0, 0);
        return;
    }

    const ParkingLot &lot = lotIt->second;
    const auto &spotMap = lot.getSpots();

    if (spotMap.empty()) {
        auto *label = new QLabel("No spots found for this lot.", this);
        spotsGrid->addWidget(label, 0, 0);
        return;
    }

    std::vector<std::string> spotIds;
    spotIds.reserve(spotMap.size());

    for (const auto &pair : spotMap) {
        spotIds.push_back(pair.first);
    }

    std::sort(spotIds.begin(), spotIds.end());

    const int columns = 3;

    for (int i = 0; i < static_cast<int>(spotIds.size()); ++i) {
        const std::string &spotIdStd = spotIds[i];
        const ParkingSpot &spot = spotMap.at(spotIdStd);

        const bool isFree = !spot.isOccupied();
        const bool newOccupied = isFree;

        const QString spotId = QString::fromStdString(spotIdStd);
        const QString currentStatus = isFree ? "Free" : "Occupied";
        const QString newStatus = newOccupied ? "Occupied" : "Free";
        const QString buttonText = spotId + "\n" + currentStatus;

        QPushButton *button = createSpotButton(buttonText, isFree, this);

        connect(button, &QPushButton::clicked, this, [this, spotId, currentStatus, newOccupied, newStatus]() {
            const auto reply = QMessageBox::question(
                this,
                "Report Spot Status",
                "Lot: " + currentLotName +
                    "\nSpot: " + spotId +
                    "\nCurrent status: " + currentStatus +
                    "\n\nChange it to: " + newStatus + " ?"
                );

            if (reply == QMessageBox::Yes) {
                controller->reportSpot(currentLotId, spotId, newOccupied);
            }
        });

        const int row = i / columns;
        const int col = i % columns;
        spotsGrid->addWidget(button, row, col);
    }
}

void MapView::closeEvent(QCloseEvent *event)
{
    if (m_previousWindow) {
        m_previousWindow->show();
    }

    QMainWindow::closeEvent(event);
}
