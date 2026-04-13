#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

class LotOverview;

class MainWindow : public QMainWindow
{
public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow() override = default;

private:
    LotOverview *lotOverviewWindow = nullptr;
};

#endif // MAINWINDOW_H
