#ifndef MAPVIEW_H
#define MAPVIEW_H

#include <QList>
#include <QMainWindow>
#include <QPointF>
#include <QPushButton>
#include <QString>
#include <QStringList>
#include <QSizeF>
#include <QTimer>
#include <QWidget>
#include <QVector>

class QCloseEvent;
class AppController;
class QLabel;
class QPainter;

// ─────────────────────────────────────────────────────────────
// ParkingMapWidget – custom-painted GPS-style parking lot map
// ─────────────────────────────────────────────────────────────
class ParkingMapWidget : public QWidget
{
    Q_OBJECT

public:
    struct SpotInfo {
        QString id;
        double  x = 0.0;
        double  y = 0.0;
        bool    occupied = false;
        bool    isGate = false;
    };

    explicit ParkingMapWidget(QWidget *parent = nullptr);

    void setSpots(const QList<SpotInfo> &spots);

    // Highlight a route using both the route node IDs and drawable coordinates.
    void setRoute(const QStringList &path,
                  const QList<QPointF> &routePoints,
                  const QString &destSpot);
    void clearRoute();

signals:
    void spotClicked(const QString &spotId);

protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    QPointF worldToScreen(double wx, double wy) const;
    QRectF  spotRect(const SpotInfo &s) const;
    void    computeBounds();
    double  pixelsPerWorldUnit() const;
    double  screenScaleX() const;
    double  screenScaleY() const;
    QSizeF  baySize() const;
    QVector<double> laneXs() const;
    QVector<double> laneYs() const;
    void drawArrow(QPainter &p, const QPointF &from, const QPointF &to) const;

    QList<SpotInfo> m_spots;
    QStringList     m_routePath;
    QList<QPointF>  m_routePoints;
    QString         m_destSpot;

    // world bounding box
    double m_minX = 0.0, m_maxX = 1.0, m_minY = 0.0, m_maxY = 1.0;

    static constexpr int MARGIN = 76;

    // route animation
    QTimer *m_animTimer = nullptr;
    int     m_animPhase = 0;
};

// ─────────────────────────────────────────────────────────────
// MapView – window wrapping the ParkingMapWidget
// ─────────────────────────────────────────────────────────────
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

private slots:
    void onNavigateClicked();

private:
    void rebuildMap();

    QWidget           *m_previousWindow = nullptr;
    AppController     *controller       = nullptr;
    QString            currentLotId;
    QString            currentLotName;

    ParkingMapWidget  *mapWidget      = nullptr;
    QLabel            *routeInfoLabel = nullptr;
    QPushButton       *clearRouteBtn  = nullptr;
};

#endif // MAPVIEW_H
