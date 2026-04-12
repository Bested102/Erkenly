#include "mapview.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QGridLayout>
#include <QLabel>
#include <QPushButton>
#include <QMessageBox>
#include <QCloseEvent>
#include <QFont>

static QPushButton* createSpotButton(const QString &spotName, bool isFree, QWidget *parent)
{
    auto *button = new QPushButton(spotName, parent);
    button->setMinimumSize(100, 70);

    if (isFree) {
        button->setStyleSheet(
            "QPushButton { background-color: #7CFC98; font-weight: bold; }"
            );
    } else {
        button->setStyleSheet(
            "QPushButton { background-color: #FF7F7F; font-weight: bold; }"
            );
    }

    return button;
}

MapView::MapView(const QString &lotName, QWidget *previousWindow)
    : QMainWindow(nullptr), m_previousWindow(previousWindow), currentLotName(lotName)
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

    auto *routePlaceholder = new QLabel(
        "Route guidance will appear here later.",
        this
        );

    auto *gridLayout = new QGridLayout();
    gridLayout->setSpacing(12);

    QPushButton *a1 = createSpotButton("A1\nFree", true, this);
    QPushButton *a2 = createSpotButton("A2\nOccupied", false, this);
    QPushButton *a3 = createSpotButton("A3\nFree", true, this);
    QPushButton *a4 = createSpotButton("A4\nOccupied", false, this);
    QPushButton *a5 = createSpotButton("A5\nFree", true, this);
    QPushButton *a6 = createSpotButton("A6\nOccupied", false, this);

    gridLayout->addWidget(a1, 0, 0);
    gridLayout->addWidget(a2, 0, 1);
    gridLayout->addWidget(a3, 0, 2);
    gridLayout->addWidget(a4, 1, 0);
    gridLayout->addWidget(a5, 1, 1);
    gridLayout->addWidget(a6, 1, 2);

    auto *backButton = new QPushButton("Back to Lots", this);
    backButton->setMinimumHeight(40);

    mainLayout->addWidget(titleLabel);
    mainLayout->addWidget(legendLabel);
    mainLayout->addWidget(routePlaceholder);
    mainLayout->addSpacing(10);
    mainLayout->addLayout(gridLayout);
    mainLayout->addSpacing(15);
    mainLayout->addWidget(backButton);

    auto showSpotMessage = [this](const QString &spot, const QString &status) {
        QMessageBox::information(
            this,
            "Spot Details",
            "Lot: " + currentLotName + "\nSpot: " + spot + "\nStatus: " + status
            );
    };

    connect(a1, &QPushButton::clicked, this, [showSpotMessage]() { showSpotMessage("A1", "Free"); });
    connect(a2, &QPushButton::clicked, this, [showSpotMessage]() { showSpotMessage("A2", "Occupied"); });
    connect(a3, &QPushButton::clicked, this, [showSpotMessage]() { showSpotMessage("A3", "Free"); });
    connect(a4, &QPushButton::clicked, this, [showSpotMessage]() { showSpotMessage("A4", "Occupied"); });
    connect(a5, &QPushButton::clicked, this, [showSpotMessage]() { showSpotMessage("A5", "Free"); });
    connect(a6, &QPushButton::clicked, this, [showSpotMessage]() { showSpotMessage("A6", "Occupied"); });

    connect(backButton, &QPushButton::clicked, this, [this]() {
        if (this->m_previousWindow) {
            this->m_previousWindow->show();
        }
        this->hide();
    });
}

void MapView::closeEvent(QCloseEvent *event)
{
    if (m_previousWindow) {
        m_previousWindow->show();
    }
    QMainWindow::closeEvent(event);
}
