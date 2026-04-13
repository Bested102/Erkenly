#include "AppController.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include "ParkingLot.hpp"
#include "ParkingSpot.hpp"
#include "Gate.hpp"

AppController::AppController(QObject *parent)
    : QObject(parent)
{
    connect(&client, &ParkingClient::connected, this, &AppController::onConnected);
    connect(&client, &ParkingClient::disconnected, this, &AppController::onDisconnected);
    connect(&client, &ParkingClient::snapshotReceived, this, &AppController::onSnapshotReceived);
    connect(&client, &ParkingClient::ackReceived, this, &AppController::onAckReceived);
    connect(&client, &ParkingClient::errorOccurred, this, &AppController::onClientError);
}

void AppController::connectToServer(const QString &host, quint16 port)
{
    connectionStatus = "Connecting...";
    emit connectionStatusChanged(connectionStatus);
    client.connectToServer(host, port);
}

void AppController::requestSnapshot()
{
    client.requestSnapshot();
}

void AppController::reportSpot(const QString &lotId, const QString &spotId, bool occupied)
{
    client.reportSpot(lotId, spotId, occupied);
}

const ParkingModel& AppController::getModel() const
{
    return model;
}

QString AppController::getConnectionStatus() const
{
    return connectionStatus;
}

void AppController::onConnected()
{
    connectionStatus = "Connected";
    emit connectionStatusChanged(connectionStatus);
    requestSnapshot();
}

void AppController::onDisconnected()
{
    connectionStatus = "Disconnected";
    emit connectionStatusChanged(connectionStatus);
}

void AppController::onSnapshotReceived(const QByteArray &data)
{
    rebuildModelFromSnapshot(data);
    emit modelChanged();
}

void AppController::onAckReceived(const QByteArray &data)
{
    Q_UNUSED(data);
    requestSnapshot();
}

void AppController::onClientError(const QString &message)
{
    emit errorOccurred(message);
}

void AppController::rebuildModelFromSnapshot(const QByteArray &data)
{
    const QJsonDocument doc = QJsonDocument::fromJson(data);
    if (!doc.isObject()) {
        emit errorOccurred("Invalid snapshot JSON.");
        return;
    }

    const QJsonObject root = doc.object();
    if (root.value("type").toString() != "snapshot") {
        emit errorOccurred("Unexpected response type.");
        return;
    }

    model.clear();

    const QJsonArray lotsArray = root.value("lots").toArray();

    for (const QJsonValue &lotValue : lotsArray) {
        const QJsonObject lotObj = lotValue.toObject();

        const std::string lotId = lotObj.value("id").toString().toStdString();
        const std::string lotName = lotObj.value("name").toString().toStdString();

        ParkingLot lot(lotId, lotName);

        const QJsonArray spotsArray = lotObj.value("spots").toArray();
        for (const QJsonValue &spotValue : spotsArray) {
            const QJsonObject spotObj = spotValue.toObject();

            ParkingSpot spot(
                spotObj.value("id").toString().toStdString(),
                spotObj.value("lotId").toString().toStdString(),
                spotObj.value("x").toDouble(),
                spotObj.value("y").toDouble(),
                spotObj.value("occupied").toBool(),
                spotObj.value("graphNodeId").toInt(-1)
                );

            lot.addSpot(spot);
        }

        const QJsonArray gatesArray = lotObj.value("gates").toArray();
        for (const QJsonValue &gateValue : gatesArray) {
            const QJsonObject gateObj = gateValue.toObject();

            Gate gate(
                gateObj.value("id").toString().toStdString(),
                gateObj.value("lotId").toString().toStdString(),
                gateObj.value("x").toDouble(),
                gateObj.value("y").toDouble(),
                gateObj.value("graphNodeId").toInt(-1)
                );

            lot.addGate(gate);
        }

        model.addLot(lot);
    }
}
