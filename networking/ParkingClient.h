#pragma once

#include "IParkingClient.h"

#include <QTcpSocket>
#include <QString>

class ParkingClient : public IParkingClient
{
    Q_OBJECT

public:
    explicit ParkingClient(QObject *parent = nullptr);

    void connectToServer(const QString &host, quint16 port) override;
    void requestSnapshot() override;
    void reportSpot(const QString &lotId, const QString &spotId, bool occupied) override;
    bool isConnected() const override;

private slots:
    void onReadyRead();
    void onConnected();
    void onDisconnected();
    void onErrorOccurred(QAbstractSocket::SocketError socketError);

private:
    QTcpSocket socket;
    QByteArray buffer;
};