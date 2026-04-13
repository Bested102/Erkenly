#include "ParkingClient.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>

ParkingClient::ParkingClient(QObject *parent)
    : QObject(parent)
{
    connect(&socket, &QTcpSocket::readyRead, this, &ParkingClient::onReadyRead);
    connect(&socket, &QTcpSocket::connected, this, &ParkingClient::onConnected);
    connect(&socket, &QTcpSocket::disconnected, this, &ParkingClient::onDisconnected);
    connect(&socket, &QTcpSocket::errorOccurred, this, &ParkingClient::onErrorOccurred);
}

void ParkingClient::connectToServer(const QString &host, quint16 port)
{
    if (socket.state() != QAbstractSocket::UnconnectedState) {
        socket.abort();
    }

    buffer.clear();
    socket.connectToHost(host, port);
}

void ParkingClient::requestSnapshot()
{
    if (socket.state() != QAbstractSocket::ConnectedState) {
        emit errorOccurred("Not connected to server.");
        return;
    }

    QJsonObject request;
    request["type"] = "get_snapshot";

    const QByteArray payload = QJsonDocument(request).toJson(QJsonDocument::Compact) + '\n';
    socket.write(payload);
    socket.flush();
}

void ParkingClient::reportSpot(const QString &lotId, const QString &spotId, bool occupied)
{
    if (socket.state() != QAbstractSocket::ConnectedState) {
        emit errorOccurred("Not connected to server.");
        return;
    }

    QJsonObject request;
    request["type"] = "report_spot";
    request["lotId"] = lotId;
    request["spotId"] = spotId;
    request["occupied"] = occupied;

    const QByteArray payload = QJsonDocument(request).toJson(QJsonDocument::Compact) + '\n';
    socket.write(payload);
    socket.flush();
}

bool ParkingClient::isConnected() const
{
    return socket.state() == QAbstractSocket::ConnectedState;
}

void ParkingClient::onReadyRead()
{
    buffer += socket.readAll();

    while (true) {
        const int newlineIndex = buffer.indexOf('\n');
        if (newlineIndex < 0) {
            break;
        }

        const QByteArray line = buffer.left(newlineIndex).trimmed();
        buffer.remove(0, newlineIndex + 1);

        if (line.isEmpty()) {
            continue;
        }

        QJsonParseError parseError;
        const QJsonDocument doc = QJsonDocument::fromJson(line, &parseError);

        if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
            emit errorOccurred("Invalid response JSON from server.");
            continue;
        }

        const QJsonObject obj = doc.object();
        const QString type = obj.value("type").toString();

        if (type == "snapshot") {
            emit snapshotReceived(line);
        } else if (type == "ack") {
            emit ackReceived(line);
        } else if (type == "error") {
            emit errorOccurred(obj.value("message").toString("Unknown server error."));
        } else {
            emit errorOccurred("Unknown server response type: " + type);
        }
    }
}

void ParkingClient::onConnected()
{
    emit connected();
}

void ParkingClient::onDisconnected()
{
    emit disconnected();
}

void ParkingClient::onErrorOccurred(QAbstractSocket::SocketError)
{
    emit errorOccurred(socket.errorString());
}
