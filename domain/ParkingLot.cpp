#include "ParkingLot.hpp"

ParkingLot::ParkingLot() {}

ParkingLot::ParkingLot(const std::string& id, const std::string& name)
    : id(id), name(name)
{}

std::string ParkingLot::getId() const {
    return id;
}

std::string ParkingLot::getName() const {
    return name;
}

void ParkingLot::addSpot(const ParkingSpot& spot) {
    spots[spot.getId()] = spot;
}

void ParkingLot::addGate(const Gate& gate) {
    gates.push_back(gate);
}

ParkingSpot* ParkingLot::getSpot(const std::string& spotId) {

    auto it = spots.find(spotId);

    if (it != spots.end())
        return &it->second;

    return nullptr;
}

std::vector<Gate>& ParkingLot::getGates() {
    return gates;
}

int ParkingLot::getTotalSpots() const {
    return spots.size();
}

int ParkingLot::getFreeSpots() const {

    int count = 0;

    for (const auto& pair : spots) {
        if (!pair.second.isOccupied())
            count++;
    }

    return count;
}

const std::unordered_map<std::string, ParkingSpot>& ParkingLot::getSpots() const {
    return spots;
}

double ParkingLot::getOccupancyPercent() const {
    const int total = getTotalSpots();
    if (total == 0) {
        return 0.0;
    }

    const int occupied = total - getFreeSpots();
    return (occupied * 100.0) / total;
}
