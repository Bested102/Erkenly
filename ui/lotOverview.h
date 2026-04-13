#ifndef LOTOVERVIEW_H
#define LOTOVERVIEW_H

#include <QMainWindow>

class QTableWidget;
class MapView;
class QCloseEvent;

class LotOverview : public QMainWindow
{
public:
    explicit LotOverview(QWidget *previousWindow = nullptr);
    ~LotOverview() override = default;

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QWidget *m_previousWindow = nullptr;
    QTableWidget *lotsTable = nullptr;
    MapView *mapViewWindow = nullptr;
};

#endif // LOTOVERVIEW_H
