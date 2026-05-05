#pragma once

#include <QObject>
#include <QString>
#include <QByteArray>
#include <QtGlobal>

class IParkingClient : public QObject
{
    Q_OBJECT

public:
    explicit IParkingClient(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    ~IParkingClient() override = default;

    virtual void connectToServer(const QString &host, quint16 port) = 0;
    virtual void requestSnapshot() = 0;
    virtual void reportSpot(const QString &lotId, const QString &spotId, bool occupied) = 0;
    virtual bool isConnected() const = 0;

signals:
    void connected();
    void disconnected();
    void snapshotReceived(const QByteArray &data);
    void ackReceived(const QByteArray &data);
    void errorOccurred(const QString &message);
};