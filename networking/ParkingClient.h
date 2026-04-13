#pragma once

#include <QObject>
#include <QTcpSocket>
#include <QString>

class ParkingClient : public QObject
{
    Q_OBJECT

public:
    explicit ParkingClient(QObject *parent = nullptr);

    void connectToServer(const QString &host, quint16 port);
    void requestSnapshot();
    void reportSpot(const QString &lotId, const QString &spotId, bool occupied);
    bool isConnected() const;

signals:
    void connected();
    void disconnected();
    void snapshotReceived(const QByteArray &data);
    void ackReceived(const QByteArray &data);
    void errorOccurred(const QString &message);

private slots:
    void onReadyRead();
    void onConnected();
    void onDisconnected();
    void onErrorOccurred(QAbstractSocket::SocketError socketError);

private:
    QTcpSocket socket;
    QByteArray buffer;
};
