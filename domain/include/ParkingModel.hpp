#pragma once

#include <unordered_map>
#include <string>

#include "ParkingLot.hpp"

class ParkingModel {
public:

    ParkingModel();

    void addLot(const ParkingLot& lot);

    ParkingLot* getLot(const std::string& lotId);

    ParkingSpot* getSpot(const std::string& lotId,
                         const std::string& spotId);

    void updateSpot(const std::string& lotId,
                    const std::string& spotId,
                    bool occupied);

private:

    std::unordered_map<std::string, ParkingLot> lots;
};