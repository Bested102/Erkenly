#include "AppController.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

#include <string>
#include <unordered_map>
#include <vector>

#include "ParkingLot.hpp"
#include "ParkingSpot.hpp"
#include "Gate.hpp"
#include "routeplanner.h"

AppController::AppController(QObject *parent)
    : QObject(parent),
      realClient(this),
      client(&realClient)
{
    connectClientSignals();
}

AppController::AppController(IParkingClient *parkingClient, QObject *parent)
    : QObject(parent),
      realClient(this),
      client(parkingClient ? parkingClient : &realClient)
{
    connectClientSignals();
}

void AppController::connectClientSignals()
{
    connect(client, &IParkingClient::connected, this, &AppController::onConnected);
    connect(client, &IParkingClient::disconnected, this, &AppController::onDisconnected);
    connect(client, &IParkingClient::snapshotReceived, this, &AppController::onSnapshotReceived);
    connect(client, &IParkingClient::ackReceived, this, &AppController::onAckReceived);
    connect(client, &IParkingClient::errorOccurred, this, &AppController::onClientError);
}

void AppController::connectToServer(const QString &host, quint16 port)
{
    connectionStatus = "Connecting...";
    emit connectionStatusChanged(connectionStatus);
    client->connectToServer(host, port);
}

void AppController::requestSnapshot()
{
    client->requestSnapshot();
}

void AppController::reportSpot(const QString &lotId, const QString &spotId, bool occupied)
{
    client->reportSpot(lotId, spotId, occupied);
}

const ParkingModel& AppController::getModel() const
{
    return model;
}

ParkingModel& AppController::getModel()
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

QStringList AppController::findRouteToNearestFreeSpot(const QString &lotId,
                                                      const QString &gateId,
                                                      QString &chosenSpot) const
{
    const std::string lotIdStd = lotId.toStdString();
    const ParkingLot *lot = model.getLot(lotIdStd);

    if (!lot) {
        return {};
    }

    RoutePlanner planner(*lot);

    std::unordered_map<std::string, bool> freeSpots;

    for (const auto &pair : lot->getSpots()) {
        freeSpots[pair.first] = !pair.second.isOccupied();
    }

    std::string chosen;

    const auto path = planner.routeToNearestFreeSpot(
        gateId.toStdString(),
        chosen,
        freeSpots
    );

    chosenSpot = QString::fromStdString(chosen);

    QStringList result;

    for (const auto &node : path) {
        result << QString::fromStdString(node);
    }

    return result;
}

QStringList AppController::findRoute(const QString &lotId,
                                     const QString &startId,
                                     const QString &goalId) const
{
    const std::string lotIdStd = lotId.toStdString();
    const ParkingLot *lot = model.getLot(lotIdStd);

    if (!lot) {
        return {};
    }

    RoutePlanner planner(*lot);
    const auto path = planner.route(startId.toStdString(), goalId.toStdString());

    QStringList result;

    for (const auto &node : path) {
        result << QString::fromStdString(node);
    }

    return result;
}

QList<QPointF> AppController::routeGeometry(const QString &lotId,
                                            const QStringList &path) const
{
    QList<QPointF> points;

    const ParkingLot *lot = model.getLot(lotId.toStdString());

    if (!lot) {
        return points;
    }

    RoutePlanner planner(*lot);

    for (const QString &id : path) {
        Coordinate coordinate;

        if (planner.nodeCoordinate(id.toStdString(), coordinate)) {
            points.append(QPointF(coordinate.x, coordinate.y));
        }
    }

    return points;
}

double AppController::routeDistance(const QString &lotId,
                                    const QStringList &path) const
{
    const ParkingLot *lot = model.getLot(lotId.toStdString());

    if (!lot) {
        return 0.0;
    }

    RoutePlanner planner(*lot);

    std::vector<std::string> nodes;
    nodes.reserve(path.size());

    for (const QString &id : path) {
        nodes.push_back(id.toStdString());
    }

    return planner.pathDistance(nodes);
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