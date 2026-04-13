#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "ParkingSpot.hpp"
#include "Gate.hpp"

class ParkingLot {
public:
    ParkingLot();
    ParkingLot(const std::string& id, const std::string& name);

    std::string getId() const;
    std::string getName() const;

    void addSpot(const ParkingSpot& spot);
    void addGate(const Gate& gate);

    ParkingSpot* getSpot(const std::string& spotId);

    const std::unordered_map<std::string, ParkingSpot>& getSpots() const;
    std::vector<Gate>& getGates();

    int getTotalSpots() const;
    int getFreeSpots() const;
    double getOccupancyPercent() const;

private:
    std::string id;
    std::string name;

    std::unordered_map<std::string, ParkingSpot> spots;
    std::vector<Gate> gates;
};
