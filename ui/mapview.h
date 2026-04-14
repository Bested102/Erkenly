#ifndef MAPVIEW_H
#define MAPVIEW_H

#include <QMainWindow>
#include <QString>

class QCloseEvent;
class QGridLayout;
class AppController;
class QScrollArea;

class MapView : public QMainWindow
{
    Q_OBJECT

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

    QWidget *asphaltContainer = nullptr;
    QGridLayout *spotsGrid = nullptr;
};

#endif // MAPVIEW_H