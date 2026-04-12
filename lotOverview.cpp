#include "lotoverview.h"
#include "mapview.h"

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

LotOverview::LotOverview(QWidget *previousWindow)
    : QMainWindow(nullptr), m_previousWindow(previousWindow)
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
        "For now, this screen shows one sample lot with basic statistics.",
        this
        );

    lotsTable = new QTableWidget(this);
    lotsTable->setColumnCount(4);
    lotsTable->setRowCount(1);
    lotsTable->setHorizontalHeaderLabels(
        QStringList() << "Lot Name" << "Total Spots" << "Free Spots" << "Occupancy %"
        );

    lotsTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    lotsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    lotsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    lotsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    lotsTable->verticalHeader()->setVisible(false);

    lotsTable->setItem(0, 0, new QTableWidgetItem("Lot A"));
    lotsTable->setItem(0, 1, new QTableWidgetItem("20"));
    lotsTable->setItem(0, 2, new QTableWidgetItem("6"));
    lotsTable->setItem(0, 3, new QTableWidgetItem("70%"));

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
        if (this->m_previousWindow) {
            this->m_previousWindow->show();
        }
        this->hide();
    });

    connect(openLotButton, &QPushButton::clicked, this, [this]() {
        int selectedRow = lotsTable->currentRow();

        if (selectedRow < 0) {
            QMessageBox::information(this, "No Selection", "Please select a lot first.");
            return;
        }

        QString lotName = lotsTable->item(selectedRow, 0)->text();

        if (!mapViewWindow) {
            mapViewWindow = new MapView(lotName, this);

            connect(mapViewWindow, &QObject::destroyed, this, [this]() {
                mapViewWindow = nullptr;
            });
        }

        mapViewWindow->show();
        mapViewWindow->raise();
        mapViewWindow->activateWindow();
        this->hide();
    });

    connect(lotsTable, &QTableWidget::cellDoubleClicked, this, [this](int row, int) {
        QString lotName = lotsTable->item(row, 0)->text();

        if (!mapViewWindow) {
            mapViewWindow = new MapView(lotName, this);

            connect(mapViewWindow, &QObject::destroyed, this, [this]() {
                mapViewWindow = nullptr;
            });
        }

        mapViewWindow->show();
        mapViewWindow->raise();
        mapViewWindow->activateWindow();
        this->hide();
    });
}

void LotOverview::closeEvent(QCloseEvent *event)
{
    if (m_previousWindow) {
        m_previousWindow->show();
    }
    QMainWindow::closeEvent(event);
}
