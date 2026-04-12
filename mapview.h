#ifndef MAPVIEW_H
#define MAPVIEW_H

#include <QMainWindow>
#include <QString>

class QCloseEvent;

class MapView : public QMainWindow
{
public:
    explicit MapView(const QString &lotName, QWidget *previousWindow = nullptr);
    ~MapView() override = default;

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QWidget *m_previousWindow = nullptr;
    QString currentLotName;
};

#endif // MAPVIEW_H
