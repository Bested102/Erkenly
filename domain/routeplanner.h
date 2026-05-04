#pragma once

#include "ParkingLot.hpp"
#include "graph.h"

#include <string>
#include <unordered_map>
#include <vector>

class RoutePlanner
{
public:
    explicit RoutePlanner(const ParkingLot& lot);

    // Returns the shortest road-following path between two known nodes.
    std::vector<std::string> route(const std::string& start,
                                   const std::string& goal);

    // Returns the shortest road-following path from start to the nearest free spot.
    std::vector<std::string> routeToNearestFreeSpot(
        const std::string& start,
        std::string& chosenSpot,
        const std::unordered_map<std::string,bool>& freeSpots);

    bool nodeCoordinate(const std::string& id, Coordinate& coordinate) const;
    double pathDistance(const std::vector<std::string>& path) const;

private:
    Graph graph;

    void buildRoadGraph(const ParkingLot& lot);
    void connectNearestRoadNode(const std::string& nodeId,
                                const Coordinate& coord,
                                bool oneWayIn = false);

    std::vector<std::string> reconstructPath(
        const std::unordered_map<std::string,std::string>& prev,
        const std::string& start,
        const std::string& goal) const;

    std::unordered_map<std::string,double> dijkstra(
        const std::string& start,
        std::unordered_map<std::string,std::string>& prev) const;
};
