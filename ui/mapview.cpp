#include "mapview.h"
#include "AppController.h"
#include "ParkingLot.hpp"
#include "ParkingSpot.hpp"
#include "Gate.hpp"

#include <QCloseEvent>
#include <QFont>
#include <QHBoxLayout>
#include <QInputDialog>
#include <QLabel>
#include <QLinearGradient>
#include <QMessageBox>
#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QResizeEvent>
#include <QStringList>
#include <QTimer>
#include <QVBoxLayout>
#include <QWidget>
#include <QtGlobal>

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>

// ════════════════════════════════════════════════════════════════
//  ParkingMapWidget
// ════════════════════════════════════════════════════════════════

ParkingMapWidget::ParkingMapWidget(QWidget *parent)
    : QWidget(parent)
{
    setMinimumSize(720, 480);
    setMouseTracking(true);

    m_animTimer = new QTimer(this);
    m_animTimer->setInterval(45);
    connect(m_animTimer, &QTimer::timeout, this, [this]() {
        m_animPhase = (m_animPhase + 1) % 36;
        update();
    });
}

void ParkingMapWidget::setSpots(const QList<SpotInfo> &spots)
{
    m_spots = spots;
    computeBounds();
    update();
}

void ParkingMapWidget::setRoute(const QStringList &path,
                                const QList<QPointF> &routePoints,
                                const QString &destSpot)
{
    m_routePath = path;
    m_routePoints = routePoints;
    m_destSpot = destSpot;
    m_animPhase = 0;

    computeBounds();

    if (m_routePoints.size() >= 2) {
        m_animTimer->start();
    } else {
        m_animTimer->stop();
    }

    update();
}

void ParkingMapWidget::clearRoute()
{
    m_routePath.clear();
    m_routePoints.clear();
    m_destSpot.clear();
    m_animTimer->stop();
    computeBounds();
    update();
}

void ParkingMapWidget::computeBounds()
{
    bool hasPoint = false;

    auto includePoint = [&](double x, double y) {
        if (!hasPoint) {
            m_minX = m_maxX = x;
            m_minY = m_maxY = y;
            hasPoint = true;
            return;
        }

        m_minX = std::min(m_minX, x);
        m_maxX = std::max(m_maxX, x);
        m_minY = std::min(m_minY, y);
        m_maxY = std::max(m_maxY, y);
    };

    for (const auto &s : m_spots) {
        includePoint(s.x, s.y);
        includePoint(s.x - 0.5, s.y - 0.5);
        includePoint(s.x + 0.5, s.y + 0.5);
    }

    for (const auto &p : m_routePoints) {
        includePoint(p.x(), p.y());
    }

    if (!hasPoint) {
        m_minX = m_minY = 0.0;
        m_maxX = m_maxY = 1.0;
    }

    // GPS-style breathing room around the lot.
    const double pad = 0.65;
    m_minX -= pad;
    m_maxX += pad;
    m_minY -= pad;
    m_maxY += pad;

    if (std::abs(m_maxX - m_minX) < 0.01) {
        m_minX -= 1.0;
        m_maxX += 1.0;
    }
    if (std::abs(m_maxY - m_minY) < 0.01) {
        m_minY -= 1.0;
        m_maxY += 1.0;
    }
}

double ParkingMapWidget::screenScaleX() const
{
    const double usableWidth = std::max(160, width() - 2 * MARGIN);
    const double spanX = std::max(1.0, m_maxX - m_minX);
    return usableWidth / spanX;
}

double ParkingMapWidget::screenScaleY() const
{
    const double usableHeight = std::max(160, height() - 2 * MARGIN);
    const double spanY = std::max(1.0, m_maxY - m_minY);
    return usableHeight / spanY;
}

double ParkingMapWidget::pixelsPerWorldUnit() const
{
    // Use the smaller axis only for stroke widths.  Positions intentionally use
    // independent X/Y scaling so compact demo coordinates do not collapse into
    // one clump on wide screens.
    return std::min(screenScaleX(), screenScaleY());
}

QPointF ParkingMapWidget::worldToScreen(double wx, double wy) const
{
    // Use a uniform scale so the parking lot keeps realistic proportions.
    const double sx = screenScaleX();
    const double sy = screenScaleY();
    const double scale = std::min(sx, sy);
    const double drawnWidth = (m_maxX - m_minX) * scale;
    const double drawnHeight = (m_maxY - m_minY) * scale;
    const double originX = (width() - drawnWidth) / 2.0;
    const double originY = (height() - drawnHeight) / 2.0;

    return QPointF(originX + (wx - m_minX) * scale,
                   originY + (wy - m_minY) * scale);
}

QSizeF ParkingMapWidget::baySize() const
{
    const double scale = pixelsPerWorldUnit();

    // Parking bays should nearly fill their grid cell. Empty grid cells are the roads.
    double w = std::clamp(scale * 0.78, 54.0, 96.0);
    double h = std::clamp(scale * 0.94, 70.0, 128.0);

    if (h < w * 1.18) {
        h = w * 1.18;
    }

    return QSizeF(w, h);
}

QRectF ParkingMapWidget::spotRect(const SpotInfo &s) const
{
    const QPointF c = worldToScreen(s.x, s.y);
    const QSizeF size = baySize();
    return QRectF(c.x() - size.width() / 2.0,
                  c.y() - size.height() / 2.0,
                  size.width(),
                  size.height());
}

QVector<double> ParkingMapWidget::laneXs() const
{
    std::set<double> values;
    for (const auto &s : m_spots) {
        values.insert(s.x - 0.5);
        values.insert(s.x + 0.5);
    }

    QVector<double> result;
    result.reserve(static_cast<int>(values.size()));
    for (double value : values) {
        result.append(value);
    }
    return result;
}

QVector<double> ParkingMapWidget::laneYs() const
{
    std::set<double> values;
    for (const auto &s : m_spots) {
        values.insert(s.y - 0.5);
        values.insert(s.y + 0.5);
    }

    QVector<double> result;
    result.reserve(static_cast<int>(values.size()));
    for (double value : values) {
        result.append(value);
    }
    return result;
}

void ParkingMapWidget::drawArrow(QPainter &p, const QPointF &from, const QPointF &to) const
{
    QPointF dir = to - from;
    const double len = std::hypot(dir.x(), dir.y());
    if (len < 24.0) {
        return;
    }

    dir /= len;
    const QPointF perp(-dir.y(), dir.x());
    const QPointF tip = from + dir * (len * 0.72);
    const QPointF base = tip - dir * 16.0;

    QPolygonF arrow;
    arrow << tip << base + perp * 7.0 << base - perp * 7.0;

    p.setPen(Qt::NoPen);
    p.setBrush(QColor(36, 129, 255));
    p.drawPolygon(arrow);
}

void ParkingMapWidget::paintEvent(QPaintEvent *)
{
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::TextAntialiasing);

    p.fillRect(rect(), QColor(124, 128, 130));

    QLinearGradient bg(0, 0, 0, height());
    bg.setColorAt(0.0, QColor(150, 154, 156));
    bg.setColorAt(1.0, QColor(110, 114, 116));
    p.fillRect(rect(), bg);

    std::set<std::pair<int,int>> occupied;
    bool haveBounds = false;
    int minX = 0, maxX = 0, minY = 0, maxY = 0;
    auto include = [&](int x, int y) {
        if (!haveBounds) {
            minX = maxX = x;
            minY = maxY = y;
            haveBounds = true;
            return;
        }
        minX = std::min(minX, x);
        maxX = std::max(maxX, x);
        minY = std::min(minY, y);
        maxY = std::max(maxY, y);
    };

    for (const auto &s : m_spots) {
        const int gx = static_cast<int>(std::lround(s.x));
        const int gy = static_cast<int>(std::lround(s.y));
        include(gx, gy);
        if (!s.isGate) {
            occupied.insert({gx, gy});
        }
    }

    if (!haveBounds) {
        return;
    }

    minX -= 1;
    maxX += 1;
    minY -= 1;
    maxY += 1;

    auto cellRect = [&](int gx, int gy) {
        QRectF r(worldToScreen(gx - 0.5, gy - 0.5), worldToScreen(gx + 0.5, gy + 0.5));
        return r.normalized();
    };

    auto isRoad = [&](int gx, int gy) {
        return occupied.find({gx, gy}) == occupied.end();
    };

    QRectF lotRect = cellRect(minX, minY).united(cellRect(maxX, maxY));
    lotRect = lotRect.adjusted(4, 4, -4, -4);

    p.setPen(QPen(QColor(240, 240, 240), 4));
    p.setBrush(QColor(142, 146, 148));
    p.drawRect(lotRect);

    // Draw driveable road cells on empty coordinates, not between every pair of spots.
    p.setPen(Qt::NoPen);
    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            if (!isRoad(x, y)) {
                continue;
            }

            QRectF road = cellRect(x, y).adjusted(2, 2, -2, -2);
            p.setBrush(QColor(133, 137, 139));
            p.drawRect(road);

            const bool leftRoad = isRoad(x - 1, y);
            const bool rightRoad = isRoad(x + 1, y);
            const bool upRoad = isRoad(x, y - 1);
            const bool downRoad = isRoad(x, y + 1);

            // Light center markings for visually obvious aisles.
            if (leftRoad || rightRoad) {
                p.setPen(QPen(QColor(246, 246, 246, 120), 2, Qt::DashLine, Qt::FlatCap));
                QPen dashed = p.pen();
                dashed.setDashPattern({8, 12});
                p.setPen(dashed);
                p.drawLine(QPointF(road.left() + 8, road.center().y()), QPointF(road.right() - 8, road.center().y()));
                p.setPen(Qt::NoPen);
            }
            if ((upRoad || downRoad) && !(leftRoad || rightRoad)) {
                p.setPen(QPen(QColor(246, 246, 246, 95), 2, Qt::DashLine, Qt::FlatCap));
                QPen dashed = p.pen();
                dashed.setDashPattern({8, 12});
                p.setPen(dashed);
                p.drawLine(QPointF(road.center().x(), road.top() + 8), QPointF(road.center().x(), road.bottom() - 8));
                p.setPen(Qt::NoPen);
            }
        }
    }

    // Route path follows the empty road cells.
    if (m_routePoints.size() >= 2) {
        QPainterPath routePath;
        routePath.moveTo(worldToScreen(m_routePoints.first().x(), m_routePoints.first().y()));
        for (int i = 1; i < m_routePoints.size(); ++i) {
            routePath.lineTo(worldToScreen(m_routePoints[i].x(), m_routePoints[i].y()));
        }

        p.setPen(QPen(QColor(17, 98, 226, 60), 18, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPath(routePath);
        p.setPen(QPen(QColor(33, 120, 255), 8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        p.drawPath(routePath);

        QPen guidePen(QColor(255, 255, 255), 3, Qt::DashLine, Qt::RoundCap, Qt::RoundJoin);
        guidePen.setDashPattern({8, 13});
        guidePen.setDashOffset(-m_animPhase);
        p.setPen(guidePen);
        p.drawPath(routePath);

        for (int i = 1; i < m_routePoints.size(); ++i) {
            drawArrow(p,
                      worldToScreen(m_routePoints[i - 1].x(), m_routePoints[i - 1].y()),
                      worldToScreen(m_routePoints[i].x(), m_routePoints[i].y()));
        }
    }

    auto carColorForId = [](const QString &id) {
        static const QVector<QColor> palette {
            QColor(50, 124, 204),
            QColor(243, 196, 64),
            QColor(236, 85, 68),
            QColor(117, 204, 77)
        };
        uint hash = 0;
        for (const QChar ch : id) {
            hash = hash * 131u + ch.unicode();
        }
        return palette.at(static_cast<int>(hash % static_cast<uint>(palette.size())));
    };

    auto drawCar = [&](const QRectF &stall, bool openUp, const QColor &bodyColor) {
        QRectF car = stall.adjusted(stall.width() * 0.18, stall.height() * 0.11,
                                    -stall.width() * 0.18, -stall.height() * 0.11);

        p.setPen(Qt::NoPen);
        p.setBrush(bodyColor);
        p.drawRoundedRect(car, 12, 12);

        QRectF windshield = openUp
            ? QRectF(car.left() + car.width() * 0.18, car.top() + car.height() * 0.08,
                     car.width() * 0.64, car.height() * 0.20)
            : QRectF(car.left() + car.width() * 0.18, car.bottom() - car.height() * 0.28,
                     car.width() * 0.64, car.height() * 0.20);

        QRectF roof(car.left() + car.width() * 0.16, car.top() + car.height() * 0.28,
                    car.width() * 0.68, car.height() * 0.42);
        QRectF rearGlass = openUp
            ? QRectF(car.left() + car.width() * 0.22, car.bottom() - car.height() * 0.28,
                     car.width() * 0.56, car.height() * 0.16)
            : QRectF(car.left() + car.width() * 0.22, car.top() + car.height() * 0.12,
                     car.width() * 0.56, car.height() * 0.16);

        p.setBrush(QColor(53, 57, 62));
        p.drawRoundedRect(roof, 7, 7);
        p.setBrush(QColor(79, 92, 109));
        p.drawRoundedRect(windshield, 5, 5);
        p.drawRoundedRect(rearGlass, 5, 5);

        p.setBrush(QColor(40, 40, 40));
        const double wheelR = std::clamp(stall.width() * 0.075, 4.0, 6.0);
        p.drawEllipse(QPointF(car.left() + 2, car.top() + car.height() * 0.24), wheelR, wheelR * 1.25);
        p.drawEllipse(QPointF(car.right() - 2, car.top() + car.height() * 0.24), wheelR, wheelR * 1.25);
        p.drawEllipse(QPointF(car.left() + 2, car.bottom() - car.height() * 0.24), wheelR, wheelR * 1.25);
        p.drawEllipse(QPointF(car.right() - 2, car.bottom() - car.height() * 0.24), wheelR, wheelR * 1.25);
    };

    const QFont idFont("Segoe UI", 9, QFont::Bold);
    const QFont stateFont("Segoe UI", 7, QFont::DemiBold);

    for (const auto &s : m_spots) {
        if (s.isGate) {
            continue;
        }

        const int gx = static_cast<int>(std::lround(s.x));
        const int gy = static_cast<int>(std::lround(s.y));
        const bool roadUp = isRoad(gx, gy - 1);
        const bool roadDown = isRoad(gx, gy + 1);
        const bool openUp = roadUp && !roadDown;
        const bool openDown = roadDown && !roadUp;
        const bool useUp = openUp || (!openDown && ((gy - minY) % 2 == 0));
        const bool isDest = (s.id == m_destSpot);

        QRectF stall = cellRect(gx, gy).adjusted(6, 6, -6, -6);

        if (isDest) {
            p.setPen(Qt::NoPen);
            p.setBrush(QColor(26, 118, 255, 65));
            p.drawRoundedRect(stall.adjusted(-8, -8, 8, 8), 14, 14);
        }

        p.setPen(QPen(QColor(245, 245, 245), 3, Qt::SolidLine, Qt::SquareCap));
        p.drawLine(QPointF(stall.left(), stall.top()), QPointF(stall.left(), stall.bottom()));
        p.drawLine(QPointF(stall.right(), stall.top()), QPointF(stall.right(), stall.bottom()));
        if (useUp) {
            p.drawLine(QPointF(stall.left(), stall.top()), QPointF(stall.right(), stall.top()));
        } else {
            p.drawLine(QPointF(stall.left(), stall.bottom()), QPointF(stall.right(), stall.bottom()));
        }

        if (!s.occupied) {
            p.setPen(Qt::NoPen);
            p.setBrush(isDest ? QColor(26, 118, 255, 55) : QColor(50, 185, 106, 28));
            p.drawRoundedRect(stall.adjusted(4, 4, -4, -4), 8, 8);
        } else {
            drawCar(stall.adjusted(4, 4, -4, -4), useUp, carColorForId(s.id));
        }

        p.setPen(Qt::white);
        p.setFont(idFont);
        QRectF labelRect = useUp
            ? QRectF(stall.left(), stall.bottom() - 18, stall.width(), 16)
            : QRectF(stall.left(), stall.top() + 2, stall.width(), 16);
        p.drawText(labelRect, Qt::AlignCenter, s.id);

        if (!s.occupied && !isDest) {
            p.setFont(stateFont);
            QRectF openRect = useUp
                ? QRectF(stall.left(), stall.bottom() - 31, stall.width(), 12)
                : QRectF(stall.left(), stall.top() + 14, stall.width(), 12);
            p.setPen(QColor(35, 146, 88));
            p.drawText(openRect, Qt::AlignCenter, "OPEN");
        }

        if (isDest) {
            p.setPen(Qt::white);
            p.setFont(QFont("Segoe UI", 18, QFont::Black));
            p.drawText(stall.adjusted(0, 8, 0, -8), Qt::AlignCenter, "★");
        }
    }

    for (const auto &s : m_spots) {
        if (!s.isGate) {
            continue;
        }

        const QPointF c = worldToScreen(s.x, s.y);
        const double radius = 30.0;
        QPolygonF gateShape;
        for (int i = 0; i < 6; ++i) {
            const double angle = 3.14159265358979323846 / 180.0 * (60.0 * i - 30.0);
            gateShape << QPointF(c.x() + radius * std::cos(angle),
                                 c.y() + radius * std::sin(angle));
        }

        p.setPen(QPen(QColor(255, 205, 87), 4));
        p.setBrush(QColor(255, 166, 66));
        p.drawPolygon(gateShape);
        p.setPen(QColor(22, 14, 6));
        p.setFont(QFont("Segoe UI", 8, QFont::Black));
        p.drawText(QRectF(c.x() - radius, c.y() - 16, radius * 2, 14), Qt::AlignCenter, "ENTRY");
        p.setFont(QFont("Segoe UI", 10, QFont::Black));
        p.drawText(QRectF(c.x() - radius, c.y(), radius * 2, 18), Qt::AlignCenter, s.id);
    }

    QRectF signRect(lotRect.right() - 52, lotRect.bottom() - 86, 46, 46);
    p.setBrush(QColor(48, 129, 204));
    p.setPen(QPen(QColor(229, 240, 250), 3));
    p.drawRoundedRect(signRect, 6, 6);
    p.setPen(Qt::white);
    p.setFont(QFont("Segoe UI", 22, QFont::Black));
    p.drawText(signRect, Qt::AlignCenter, "P");

    QRectF overlay(18, 18, 185, 58);
    p.setBrush(QColor(39, 42, 44, 180));
    p.setPen(QPen(QColor(255, 255, 255, 26), 1));
    p.drawRoundedRect(overlay, 12, 12);
    p.setPen(Qt::white);
    p.setFont(QFont("Segoe UI", 10, QFont::Bold));
    p.drawText(overlay.adjusted(12, 8, -10, -26), Qt::AlignLeft | Qt::AlignVCenter, "PARKING LOT MAP");
    p.setFont(QFont("Segoe UI", 8));
    p.setPen(QColor(229, 235, 238));
    p.drawText(overlay.adjusted(12, 28, -10, -8), Qt::AlignLeft | Qt::AlignVCenter,
               m_routePoints.size() >= 2 ? "Route follows empty aisles" : "Tap Navigate to start");
}


void ParkingMapWidget::mousePressEvent(QMouseEvent *event)
{
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    const QPointF click = event->position();
#else
    const QPointF click = event->pos();
#endif

    for (const auto &s : m_spots) {
        if (s.isGate) {
            continue;
        }

        if (spotRect(s).contains(click)) {
            emit spotClicked(s.id);
            return;
        }
    }
}

void ParkingMapWidget::resizeEvent(QResizeEvent *)
{
    update();
}

// ════════════════════════════════════════════════════════════════
//  MapView
// ════════════════════════════════════════════════════════════════

MapView::MapView(const QString &lotId,
                 const QString &lotName,
                 AppController *controller,
                 QWidget *previousWindow)
    : QMainWindow(nullptr),
      m_previousWindow(previousWindow),
      controller(controller),
      currentLotId(lotId),
      currentLotName(lotName)
{
    setWindowTitle(lotName + " – GPS Parking Map");
    resize(1120, 760);

    QWidget *central = new QWidget(this);
    setCentralWidget(central);

    auto *mainLayout = new QVBoxLayout(central);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    QWidget *toolbar = new QWidget(this);
    toolbar->setObjectName("toolbar");
    toolbar->setFixedHeight(70);
    auto *tbLayout = new QHBoxLayout(toolbar);
    tbLayout->setContentsMargins(22, 0, 22, 0);
    tbLayout->setSpacing(12);

    auto *backBtn = new QPushButton("← Back", toolbar);
    backBtn->setObjectName("secondaryButton");
    backBtn->setFixedHeight(38);
    backBtn->setCursor(Qt::PointingHandCursor);

    auto *titleLabel = new QLabel(lotName + "  ·  GPS Map", toolbar);
    titleLabel->setObjectName("titleLabel");

    auto *navigateBtn = new QPushButton("🧭  Route to nearest open bay", toolbar);
    navigateBtn->setObjectName("primaryButton");
    navigateBtn->setFixedHeight(38);
    navigateBtn->setCursor(Qt::PointingHandCursor);

    clearRouteBtn = new QPushButton("✕  Clear", toolbar);
    clearRouteBtn->setObjectName("secondaryButton");
    clearRouteBtn->setFixedHeight(38);
    clearRouteBtn->setCursor(Qt::PointingHandCursor);
    clearRouteBtn->setVisible(false);

    tbLayout->addWidget(backBtn);
    tbLayout->addSpacing(8);
    tbLayout->addWidget(titleLabel);
    tbLayout->addStretch();
    tbLayout->addWidget(clearRouteBtn);
    tbLayout->addWidget(navigateBtn);

    routeInfoLabel = new QLabel("", central);
    routeInfoLabel->setObjectName("routeInfoLabel");
    routeInfoLabel->setWordWrap(true);
    routeInfoLabel->setVisible(false);
    routeInfoLabel->setContentsMargins(22, 9, 22, 9);

    mapWidget = new ParkingMapWidget(central);

    mainLayout->addWidget(toolbar);
    mainLayout->addWidget(routeInfoLabel);
    mainLayout->addWidget(mapWidget, 1);

    setStyleSheet(R"(
        QMainWindow { background: #08111f; }
        #toolbar {
            background-color: #0b1320;
            border-bottom: 1px solid #1d3048;
        }
        #titleLabel {
            color: #edf6ff;
            font-size: 18px;
            font-weight: 800;
            font-family: 'Segoe UI', Arial, sans-serif;
            letter-spacing: -0.2px;
        }
        #routeInfoLabel {
            background-color: #07254f;
            color: #d5eaff;
            font-size: 13px;
            font-weight: 700;
            font-family: 'Segoe UI', Arial, sans-serif;
            border-bottom: 1px solid #1f5b99;
        }
        #primaryButton {
            background-color: #1473ff;
            color: white;
            border: none;
            border-radius: 10px;
            font-weight: 800;
            font-size: 13px;
            padding: 0px 18px;
        }
        #primaryButton:hover { background-color: #0d63de; }
        #primaryButton:pressed { background-color: #0a4ead; }
        #secondaryButton {
            background-color: #111d2d;
            color: #c7d8ea;
            border: 1px solid #2b405b;
            border-radius: 10px;
            font-weight: 700;
            font-size: 13px;
            padding: 0px 15px;
        }
        #secondaryButton:hover {
            background-color: #18283d;
            color: #ffffff;
            border: 1px solid #456282;
        }
    )");

    connect(backBtn, &QPushButton::clicked, this, [this]() {
        if (m_previousWindow) m_previousWindow->show();
        hide();
    });

    connect(navigateBtn, &QPushButton::clicked, this, &MapView::onNavigateClicked);

    connect(clearRouteBtn, &QPushButton::clicked, this, [this]() {
        mapWidget->clearRoute();
        routeInfoLabel->setVisible(false);
        clearRouteBtn->setVisible(false);
    });

    connect(mapWidget, &ParkingMapWidget::spotClicked, this,
            [this](const QString &spotId) {
                ParkingLot *lot = this->controller->getModel().getLot(currentLotId.toStdString());
                if (!lot) return;

                ParkingSpot *spot = lot->getSpot(spotId.toStdString());
                if (!spot) return;

                const bool newOccupied = !spot->isOccupied();
                const QString newStatus = newOccupied ? "Occupied" : "Open";

                const auto reply = QMessageBox::question(
                    this,
                    "Update Parking Bay",
                    "Mark bay " + spotId + " as " + newStatus + "?",
                    QMessageBox::Yes | QMessageBox::No);

                if (reply == QMessageBox::Yes) {
                    this->controller->reportSpot(currentLotId, spotId, newOccupied);
                }
            });

    connect(this->controller, &AppController::modelChanged, this, [this]() {
        rebuildMap();
    });

    rebuildMap();
}

void MapView::rebuildMap()
{
    const auto &lots = controller->getModel().getLots();
    const auto it = lots.find(currentLotId.toStdString());
    if (it == lots.end()) {
        return;
    }

    const ParkingLot &lot = it->second;
    QList<ParkingMapWidget::SpotInfo> infos;

    for (const auto &pair : lot.getSpots()) {
        const ParkingSpot &s = pair.second;
        infos.append({QString::fromStdString(s.getId()),
                      s.getX(),
                      s.getY(),
                      s.isOccupied(),
                      false});
    }

    std::sort(infos.begin(), infos.end(), [](const ParkingMapWidget::SpotInfo &a,
                                               const ParkingMapWidget::SpotInfo &b) {
        if (a.isGate != b.isGate) {
            return !a.isGate; // draw bays first, then gates above them
        }
        if (std::abs(a.y - b.y) > 1e-6) {
            return a.y < b.y;
        }
        if (std::abs(a.x - b.x) > 1e-6) {
            return a.x < b.x;
        }
        return a.id < b.id;
    });

    for (const auto &g : lot.getGates()) {
        infos.append({QString::fromStdString(g.getId()),
                      g.getX(),
                      g.getY(),
                      false,
                      true});
    }

    mapWidget->setSpots(infos);
}

void MapView::onNavigateClicked()
{
    const auto &lots = controller->getModel().getLots();
    const auto it = lots.find(currentLotId.toStdString());
    if (it == lots.end()) {
        return;
    }

    const ParkingLot &lot = it->second;

    if (lot.getFreeSpots() == 0) {
        QMessageBox::information(this, "Navigate", "All bays are occupied. No route is available.");
        return;
    }

    const auto &gates = lot.getGates();
    if (gates.empty()) {
        QMessageBox::information(this, "Navigate", "This lot has no entry gates defined.");
        return;
    }

    QString gateId;
    if (gates.size() == 1) {
        gateId = QString::fromStdString(gates[0].getId());
    } else {
        QStringList ids;
        for (const auto &g : gates) {
            ids << QString::fromStdString(g.getId());
        }

        bool ok = false;
        gateId = QInputDialog::getItem(this,
                                       "Select Entry Gate",
                                       "Which gate are you entering from?",
                                       ids,
                                       0,
                                       false,
                                       &ok);
        if (!ok || gateId.isEmpty()) {
            return;
        }
    }

    QString chosenSpot;
    const QStringList path = controller->findRouteToNearestFreeSpot(currentLotId, gateId, chosenSpot);
    const QList<QPointF> routePoints = controller->routeGeometry(currentLotId, path);

    if (path.isEmpty() || routePoints.size() < 2) {
        QMessageBox::warning(this,
                             "Navigate",
                             "No driveable route was found from gate " + gateId + ".");
        return;
    }

    mapWidget->setRoute(path, routePoints, chosenSpot);

    QStringList visibleSteps;
    for (const QString &node : path) {
        if (!node.startsWith("R_")) {
            visibleSteps << node;
        }
    }

    const double distance = controller->routeDistance(currentLotId, path);
    routeInfoLabel->setText(
        QString("  🧭 Fastest route found   •   Entry: %1   •   Destination: %2   •   Distance: %3 map units   •   Path: %4")
            .arg(gateId)
            .arg(chosenSpot)
            .arg(distance, 0, 'f', 2)
            .arg(visibleSteps.join(" → ")));
    routeInfoLabel->setVisible(true);
    clearRouteBtn->setVisible(true);
}

void MapView::closeEvent(QCloseEvent *event)
{
    if (m_previousWindow) {
        m_previousWindow->show();
    }
    QMainWindow::closeEvent(event);
}
