#include "mainwindow.h"
#include "lotoverview.h"

#include <QWidget>
#include <QVBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QSpacerItem>
#include <QFont>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    setWindowTitle("Campus Parking Navigator");
    resize(800, 500);

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
        "View campus parking lots and open a lot map",
        this
        );
    subtitleLabel->setAlignment(Qt::AlignCenter);

    auto *viewLotsButton = new QPushButton("View Parking Lots", this);
    viewLotsButton->setMinimumHeight(45);

    auto *exitButton = new QPushButton("Exit", this);
    exitButton->setMinimumHeight(45);

    layout->addWidget(titleLabel);
    layout->addWidget(subtitleLabel);
    layout->addSpacing(20);
    layout->addWidget(viewLotsButton);
    layout->addWidget(exitButton);

    layout->addStretch();

    connect(viewLotsButton, &QPushButton::clicked, this, [this]() {
        if (!lotOverviewWindow) {
            lotOverviewWindow = new LotOverview(this);

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
}
