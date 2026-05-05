#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <QString>
#include <QByteArray>

#include "AppController.h"
#include "IParkingClient.h"

using ::testing::_;
using ::testing::Exactly;

class MockParkingClient : public IParkingClient
{
public:
    explicit MockParkingClient(QObject *parent = nullptr)
        : IParkingClient(parent)
    {
    }

    MOCK_METHOD(void, connectToServer, (const QString &host, quint16 port), (override));
    MOCK_METHOD(void, requestSnapshot, (), (override));
    MOCK_METHOD(void, reportSpot, (const QString &lotId, const QString &spotId, bool occupied), (override));
    MOCK_METHOD(bool, isConnected, (), (const, override));

    void simulateConnected()
    {
        emit connected();
    }

    void simulateDisconnected()
    {
        emit disconnected();
    }

    void simulateSnapshot(const QByteArray &data)
    {
        emit snapshotReceived(data);
    }

    void simulateAck(const QByteArray &data)
    {
        emit ackReceived(data);
    }

    void simulateError(const QString &message)
    {
        emit errorOccurred(message);
    }
};

TEST(AppControllerMockTest, ConnectToServerUpdatesStatusAndCallsClient)
{
    MockParkingClient mockClient;
    AppController controller(static_cast<IParkingClient*>(&mockClient));

    EXPECT_CALL(mockClient, connectToServer(QString("127.0.0.1"), 5050))
        .Times(Exactly(1));

    controller.connectToServer("127.0.0.1", 5050);

    EXPECT_EQ(controller.getConnectionStatus().toStdString(), "Connecting...");
}

TEST(AppControllerMockTest, ConnectedSignalUpdatesStatusAndRequestsSnapshot)
{
    MockParkingClient mockClient;
    AppController controller(static_cast<IParkingClient*>(&mockClient));

    EXPECT_CALL(mockClient, requestSnapshot())
        .Times(Exactly(1));

    mockClient.simulateConnected();

    EXPECT_EQ(controller.getConnectionStatus().toStdString(), "Connected");
}

TEST(AppControllerMockTest, DisconnectedSignalUpdatesStatus)
{
    MockParkingClient mockClient;
    AppController controller(static_cast<IParkingClient*>(&mockClient));

    mockClient.simulateDisconnected();

    EXPECT_EQ(controller.getConnectionStatus().toStdString(), "Disconnected");
}

TEST(AppControllerMockTest, ReportSpotDelegatesToClient)
{
    MockParkingClient mockClient;
    AppController controller(static_cast<IParkingClient*>(&mockClient));

    EXPECT_CALL(mockClient, reportSpot(QString("lotA"), QString("A1"), true))
        .Times(Exactly(1));

    controller.reportSpot("lotA", "A1", true);
}

TEST(AppControllerMockTest, AckSignalRequestsSnapshotAgain)
{
    MockParkingClient mockClient;
    AppController controller(static_cast<IParkingClient*>(&mockClient));

    EXPECT_CALL(mockClient, requestSnapshot())
        .Times(Exactly(1));

    mockClient.simulateAck(R"({"type":"ack"})");
}

TEST(AppControllerMockTest, SnapshotSignalRebuildsModel)
{
    MockParkingClient mockClient;
    AppController controller(static_cast<IParkingClient*>(&mockClient));

    const QByteArray snapshot = R"(
    {
        "type": "snapshot",
        "lots": [
            {
                "id": "lotA",
                "name": "Lot A",
                "spots": [
                    {
                        "id": "A1",
                        "lotId": "lotA",
                        "x": 0,
                        "y": 0,
                        "occupied": false,
                        "graphNodeId": -1
                    },
                    {
                        "id": "A2",
                        "lotId": "lotA",
                        "x": 1,
                        "y": 0,
                        "occupied": true,
                        "graphNodeId": -1
                    }
                ],
                "gates": [
                    {
                        "id": "G1",
                        "lotId": "lotA",
                        "x": -1,
                        "y": 0,
                        "graphNodeId": -1
                    }
                ]
            }
        ]
    }
    )";

    mockClient.simulateSnapshot(snapshot);

    const ParkingLot *lot = controller.getModel().getLot("lotA");

    ASSERT_NE(lot, nullptr);
    EXPECT_EQ(lot->getId(), "lotA");
    EXPECT_EQ(lot->getName(), "Lot A");
    EXPECT_EQ(lot->getTotalSpots(), 2);
    EXPECT_EQ(lot->getFreeSpots(), 1);

    ParkingSpot *spotA1 = controller.getModel().getSpot("lotA", "A1");
    ParkingSpot *spotA2 = controller.getModel().getSpot("lotA", "A2");

    ASSERT_NE(spotA1, nullptr);
    ASSERT_NE(spotA2, nullptr);

    EXPECT_FALSE(spotA1->isOccupied());
    EXPECT_TRUE(spotA2->isOccupied());
}