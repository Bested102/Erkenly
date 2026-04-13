#ifndef MAPVIEW_H
#define MAPVIEW_H

#include <QMainWindow>
#include <QString>

class QCloseEvent;
class QGridLayout;
class AppController;

class MapView : public QMainWindow
{
public:
    explicit MapView(const QString &lotId,
                     const QString &lotName,
                     AppController *controller,
                     QWidget *previousWindow = nullptr);
    ~MapView() override = default;

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    void rebuildSpots();

    QWidget *m_previousWindow = nullptr;
    AppController *controller = nullptr;

    QString currentLotId;
    QString currentLotName;

    QGridLayout *spotsGrid = nullptr;
};

#endif // MAPVIEW_H
