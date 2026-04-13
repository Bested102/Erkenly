#pragma once
#include <string>

class ParkingSpot {
public:

    ParkingSpot();

    ParkingSpot(const std::string& id,
                const std::string& lotId,
                double x,
                double y,
                bool occupied = false,
                int graphNodeId = -1);

    std::string getId() const;
    std::string getLotId() const;

    double getX() const;
    double getY() const;

    bool isOccupied() const;
    void setOccupied(bool status);

    int getGraphNodeId() const;
    void setGraphNodeId(int nodeId);

private:

    std::string id;
    std::string lotId;

    double x;
    double y;

    bool occupied;

    int graphNodeId;
};