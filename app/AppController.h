#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QPointF>

#include "ParkingModel.hpp"
#include "ParkingClient.h"
#include "routeplanner.h"

class AppController : public QObject
{
    Q_OBJECT

public:
    explicit AppController(QObject *parent = nullptr);

    void connectToServer(const QString &host, quint16 port);
    void requestSnapshot();
    void reportSpot(const QString &lotId, const QString &spotId, bool occupied);

    const ParkingModel& getModel() const;
    ParkingModel& getModel();
    QString getConnectionStatus() const;

    // Route planning: returns path node IDs from gateId to nearest free spot.
    // chosenSpot is set to the target spot ID. Returns empty list if no route found.
    QStringList findRouteToNearestFreeSpot(const QString &lotId,
                                           const QString &gateId,
                                           QString &chosenSpot) const;

    // Route planning: returns path node IDs from start to goal.
    QStringList findRoute(const QString &lotId,
                          const QString &startId,
                          const QString &goalId) const;

    // Converts a route node list into drawable map coordinates.
    QList<QPointF> routeGeometry(const QString &lotId,
                                 const QStringList &path) const;

    double routeDistance(const QString &lotId,
                         const QStringList &path) const;

signals:
    void modelChanged();
    void connectionStatusChanged(const QString &status);
    void errorOccurred(const QString &message);

private slots:
    void onConnected();
    void onDisconnected();
    void onSnapshotReceived(const QByteArray &data);
    void onAckReceived(const QByteArray &data);
    void onClientError(const QString &message);

private:
    void rebuildModelFromSnapshot(const QByteArray &data);

    ParkingModel model;
    ParkingClient client;
    QString connectionStatus = "Disconnected";
};
