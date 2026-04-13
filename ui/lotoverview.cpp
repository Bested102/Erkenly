#include "lotoverview.h"
#include "mapview.h"
#include "AppController.h"
#include "ParkingLot.hpp"

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QMessageBox>
#include <QCloseEvent>
#include <QFont>
#include <QTableWidgetItem>

LotOverview::LotOverview(AppController *appController, QWidget *previousWindow)
    : QMainWindow(nullptr),
    m_previousWindow(previousWindow),
    controller(appController)
{
    setWindowTitle("Lot Overview");
    resize(900, 550);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(30, 30, 30, 30);
    mainLayout->setSpacing(15);

    auto *titleLabel = new QLabel("Parking Lots Overview", this);
    QFont titleFont;
    titleFont.setPointSize(18);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);

    auto *infoLabel = new QLabel(
        "This screen loads lots from the server snapshot.",
        this
        );

    lotsTable = new QTableWidget(this);
    lotsTable->setColumnCount(4);
    lotsTable->setHorizontalHeaderLabels(
        QStringList() << "Lot Name" << "Total Spots" << "Free Spots" << "Occupancy %"
        );

    lotsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    lotsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    lotsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    lotsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    lotsTable->verticalHeader()->setVisible(false);

    auto *buttonsLayout = new QHBoxLayout();

    auto *backButton = new QPushButton("Back", this);
    auto *openLotButton = new QPushButton("Open Selected Lot", this);

    backButton->setMinimumHeight(40);
    openLotButton->setMinimumHeight(40);

    buttonsLayout->addWidget(backButton);
    buttonsLayout->addStretch();
    buttonsLayout->addWidget(openLotButton);

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(infoLabel);
    mainLayout->addWidget(lotsTable);
    mainLayout->addLayout(buttonsLayout);

    connect(backButton, &QPushButton::clicked, this, [this]() {
        if (m_previousWindow) {
            m_previousWindow->show();
        }
        hide();
    });

    connect(openLotButton, &QPushButton::clicked, this, [this]() {
        int selectedRow = lotsTable->currentRow();

        if (selectedRow < 0) {
            QMessageBox::information(this, "No Selection", "Please select a lot first.");
            return;
        }

        const QString lotId = lotsTable->item(selectedRow, 0)->data(Qt::UserRole).toString();
        const QString lotName = lotsTable->item(selectedRow, 0)->text();

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

    connect(controller, &AppController::modelChanged, this, [this]() {
        refreshTable();
    });

    refreshTable();
}

void LotOverview::refreshTable()
{
    lotsTable->setRowCount(0);

    int row = 0;
    for (const auto &pair : controller->getModel().getLots()) {
        const ParkingLot &lot = pair.second;

        lotsTable->insertRow(row);

        auto *nameItem = new QTableWidgetItem(QString::fromStdString(lot.getName()));
        nameItem->setData(Qt::UserRole, QString::fromStdString(lot.getId()));

        lotsTable->setItem(row, 0, nameItem);
        lotsTable->setItem(row, 1, new QTableWidgetItem(QString::number(lot.getTotalSpots())));
        lotsTable->setItem(row, 2, new QTableWidgetItem(QString::number(lot.getFreeSpots())));
        lotsTable->setItem(
            row,
            3,
            new QTableWidgetItem(QString::number(lot.getOccupancyPercent(), 'f', 1) + "%")
            );

        ++row;
    }
}

void LotOverview::closeEvent(QCloseEvent *event)
{
    if (m_previousWindow) {
        m_previousWindow->show();
    }

    QMainWindow::closeEvent(event);
}
