#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class LotOverview;
class AppController;
class QLabel;

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private:
    AppController *controller = nullptr;
    LotOverview *lotOverviewWindow = nullptr;
    QLabel *statusLabel = nullptr;
};

#endif // MAINWINDOW_H
