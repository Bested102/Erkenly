#include "ParkingModel.hpp"

ParkingModel::ParkingModel() {}

void ParkingModel::addLot(const ParkingLot& lot) {
    lots[lot.getId()] = lot;
}

ParkingLot* ParkingModel::getLot(const std::string& lotId) {

    auto it = lots.find(lotId);

    if (it != lots.end())
        return &it->second;

    return nullptr;
}

ParkingSpot* ParkingModel::getSpot(const std::string& lotId,
                                   const std::string& spotId) {

    ParkingLot* lot = getLot(lotId);

    if (!lot)
        return nullptr;

    return lot->getSpot(spotId);
}

void ParkingModel::updateSpot(const std::string& lotId,
                              const std::string& spotId,
                              bool occupied) {

    ParkingSpot* spot = getSpot(lotId, spotId);

    if (spot)
        spot->setOccupied(occupied);
}