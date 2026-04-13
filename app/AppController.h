#pragma once

#include <QObject>
#include <QString>

#include "ParkingModel.hpp"
#include "ParkingClient.h"

class AppController : public QObject
{
    Q_OBJECT

public:
    explicit AppController(QObject *parent = nullptr);

    void connectToServer(const QString &host, quint16 port);
    void requestSnapshot();
    void reportSpot(const QString &lotId, const QString &spotId, bool occupied);

    const ParkingModel& getModel() const;
    QString getConnectionStatus() const;

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
