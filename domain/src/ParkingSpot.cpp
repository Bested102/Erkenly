#include "ParkingSpot.hpp"

ParkingSpot::ParkingSpot()
    : x(0), y(0), occupied(false), graphNodeId(-1)
{}

ParkingSpot::ParkingSpot(const std::string& id,
                         const std::string& lotId,
                         double x,
                         double y,
                         bool occupied,
                         int graphNodeId)
    : id(id),
      lotId(lotId),
      x(x),
      y(y),
      occupied(occupied),
      graphNodeId(graphNodeId)
{}

std::string ParkingSpot::getId() const {
    return id;
}

std::string ParkingSpot::getLotId() const {
    return lotId;
}

double ParkingSpot::getX() const {
    return x;
}

double ParkingSpot::getY() const {
    return y;
}

bool ParkingSpot::isOccupied() const {
    return occupied;
}

void ParkingSpot::setOccupied(bool status) {
    occupied = status;
}

int ParkingSpot::getGraphNodeId() const {
    return graphNodeId;
}

void ParkingSpot::setGraphNodeId(int nodeId) {
    graphNodeId = nodeId;
}