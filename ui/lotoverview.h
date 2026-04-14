#ifndef LOTOVERVIEW_H
#define LOTOVERVIEW_H

#include <QMainWindow>

class QGridLayout;
class QScrollArea;
class MapView;
class QCloseEvent;
class AppController;

class LotOverview : public QMainWindow
{
    Q_OBJECT

public:
    explicit LotOverview(AppController *controller, QWidget *previousWindow = nullptr);
    ~LotOverview() override = default;

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void refreshGrid(); // Renamed for accuracy

    QWidget *m_previousWindow = nullptr;
    AppController *controller = nullptr;
    QGridLayout *gridLayout = nullptr; // Replaced table with a grid
    MapView *mapViewWindow = nullptr;
};

#endif // LOTOVERVIEW_H