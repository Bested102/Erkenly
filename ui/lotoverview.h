#ifndef LOTOVERVIEW_H
#define LOTOVERVIEW_H

#include <QMainWindow>

class QTableWidget;
class MapView;
class QCloseEvent;
class AppController;

class LotOverview : public QMainWindow
{
public:
    explicit LotOverview(AppController *controller, QWidget *previousWindow = nullptr);
    ~LotOverview() override = default;

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void refreshTable();

    QWidget *m_previousWindow = nullptr;
    AppController *controller = nullptr;
    QTableWidget *lotsTable = nullptr;
    MapView *mapViewWindow = nullptr;
};

#endif // LOTOVERVIEW_H
