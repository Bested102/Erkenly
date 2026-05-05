#pragma once

#include <QObject>
#include <QString>
#include <QStringList>
#include <QList>
#include <QPointF>

#include "ParkingModel.hpp"
#include "ParkingClient.h"
#include "IParkingClient.h"
#include "routeplanner.h"

class AppController : public QObject
{
    Q_OBJECT

public:
    explicit AppController(QObject *parent = nullptr);

    explicit AppController(IParkingClient *parkingClient, QObject *parent = nullptr);

    void connectToServer(const QString &host, quint16 port);
    void requestSnapshot();
    void reportSpot(const QString &lotId, const QString &spotId, bool occupied);

    const ParkingModel& getModel() const;
    ParkingModel& getModel();
    QString getConnectionStatus() const;

    QStringList findRouteToNearestFreeSpot(const QString &lotId,
                                           const QString &gateId,
                                           QString &chosenSpot) const;

    QStringList findRoute(const QString &lotId,
                          const QString &startId,
                          const QString &goalId) const;

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
    void connectClientSignals();
    void rebuildModelFromSnapshot(const QByteArray &data);

    ParkingModel model;

    ParkingClient realClient;

    IParkingClient *client = nullptr;

    QString connectionStatus = "Disconnected";
};