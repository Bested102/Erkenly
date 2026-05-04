#include "routeplanner.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <queue>
#include <set>
#include <sstream>

namespace
{
constexpr double kInfinity = std::numeric_limits<double>::infinity();

std::string roadNodeId(int x, int y)
{
    return "R_" + std::to_string(x) + "_" + std::to_string(y);
}

std::pair<int,int> toGridCell(const Coordinate& c)
{
    return {static_cast<int>(std::lround(c.x)), static_cast<int>(std::lround(c.y))};
}

void addUndirectedEdge(Graph& graph,
                       const std::string& from,
                       const std::string& to)
{
    if (from == to || !graph.hasNode(from) || !graph.hasNode(to)) {
        return;
    }

    const double distance = graph.getNode(from).coord.distanceTo(graph.getNode(to).coord);
    graph.addEdge(from, to, distance);
    graph.addEdge(to, from, distance);
}
}

RoutePlanner::RoutePlanner(const ParkingLot& lot)
{
    buildRoadGraph(lot);
}

void RoutePlanner::buildRoadGraph(const ParkingLot& lot)
{
    const auto& spots = lot.getSpots();
    const auto& gates = lot.getGates();

    std::set<std::pair<int,int>> occupiedCells;
    std::set<std::pair<int,int>> gateCells;

    bool haveBounds = false;
    int minX = 0, maxX = 0, minY = 0, maxY = 0;
    auto includeBounds = [&](int x, int y) {
        if (!haveBounds) {
            minX = maxX = x;
            minY = maxY = y;
            haveBounds = true;
            return;
        }
        minX = std::min(minX, x);
        maxX = std::max(maxX, x);
        minY = std::min(minY, y);
        maxY = std::max(maxY, y);
    };

    for (const auto& entry : spots) {
        const auto& spot = entry.second;
        const auto cell = std::make_pair(static_cast<int>(std::lround(spot.getX())),
                                         static_cast<int>(std::lround(spot.getY())));
        occupiedCells.insert(cell);
        includeBounds(cell.first, cell.second);
        graph.addNode(spot.getId(), Coordinate(spot.getX(), spot.getY()));
    }

    for (const auto& gate : gates) {
        const auto cell = std::make_pair(static_cast<int>(std::lround(gate.getX())),
                                         static_cast<int>(std::lround(gate.getY())));
        gateCells.insert(cell);
        includeBounds(cell.first, cell.second);
        graph.addNode(gate.getId(), Coordinate(gate.getX(), gate.getY()));
    }

    if (!haveBounds) {
        return;
    }

    // Expand the drivable grid around the lot so routing can happen on empty
    // grid cells, not in the gaps between parking spaces.
    minX -= 1;
    maxX += 1;
    minY -= 1;
    maxY += 1;

    auto isRoadCell = [&](int x, int y) {
        return occupiedCells.find({x, y}) == occupiedCells.end();
    };

    // Create road nodes on empty grid cells only.
    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            if (!isRoadCell(x, y)) {
                continue;
            }
            graph.addNode(roadNodeId(x, y), Coordinate(x, y));
        }
    }

    // 4-neighbor connectivity between empty cells.
    constexpr int dirs[4][2] = {{1,0},{-1,0},{0,1},{0,-1}};
    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            if (!isRoadCell(x, y)) {
                continue;
            }

            const std::string from = roadNodeId(x, y);
            for (const auto& d : dirs) {
                const int nx = x + d[0];
                const int ny = y + d[1];
                if (nx < minX || nx > maxX || ny < minY || ny > maxY) {
                    continue;
                }
                if (!isRoadCell(nx, ny)) {
                    continue;
                }
                addUndirectedEdge(graph, from, roadNodeId(nx, ny));
            }
        }
    }

    // Parking spots connect only to adjacent empty road cells. They remain
    // dead-end destinations so the planner cannot shortcut through bays.
    for (const auto& entry : spots) {
        const auto& spot = entry.second;
        const int x = static_cast<int>(std::lround(spot.getX()));
        const int y = static_cast<int>(std::lround(spot.getY()));
        const std::string spotId = spot.getId();

        for (const auto& d : dirs) {
            const int nx = x + d[0];
            const int ny = y + d[1];
            if (nx < minX || nx > maxX || ny < minY || ny > maxY) {
                continue;
            }
            if (!isRoadCell(nx, ny)) {
                continue;
            }
            const std::string roadId = roadNodeId(nx, ny);
            const double edgeDistance = graph.getNode(spotId).coord.distanceTo(graph.getNode(roadId).coord);
            graph.addEdge(roadId, spotId, edgeDistance);
        }
    }

    // Gates act as starting points. If a gate sits on a road cell, connect it
    // directly to that cell. Also connect to adjacent road cells.
    for (const auto& gate : gates) {
        const int x = static_cast<int>(std::lround(gate.getX()));
        const int y = static_cast<int>(std::lround(gate.getY()));
        const std::string gateId = gate.getId();

        if (isRoadCell(x, y) && graph.hasNode(roadNodeId(x, y))) {
            graph.addEdge(gateId, roadNodeId(x, y), 0.0);
            graph.addEdge(roadNodeId(x, y), gateId, 0.0);
        }

        for (const auto& d : dirs) {
            const int nx = x + d[0];
            const int ny = y + d[1];
            if (nx < minX || nx > maxX || ny < minY || ny > maxY) {
                continue;
            }
            if (!isRoadCell(nx, ny)) {
                continue;
            }
            const std::string roadId = roadNodeId(nx, ny);
            const double edgeDistance = graph.getNode(gateId).coord.distanceTo(graph.getNode(roadId).coord);
            graph.addEdge(gateId, roadId, edgeDistance);
            graph.addEdge(roadId, gateId, edgeDistance);
        }
    }
}

void RoutePlanner::connectNearestRoadNode(const std::string&,
                                          const Coordinate&,
                                          bool)
{
    // Legacy helper kept to preserve the header/API. Routing now uses
    // adjacency to empty grid cells instead of nearest geometric lane points.
}

std::unordered_map<std::string,double> RoutePlanner::dijkstra(
    const std::string& start,
    std::unordered_map<std::string,std::string>& prev) const
{
    std::unordered_map<std::string,double> dist;

    for (const auto& n : graph.getNodes()) {
        dist[n.first] = kInfinity;
    }

    if (!graph.hasNode(start)) {
        return dist;
    }

    dist[start] = 0.0;

    using QueueEntry = std::pair<double,std::string>;
    std::priority_queue<QueueEntry,
                        std::vector<QueueEntry>,
                        std::greater<QueueEntry>> pq;
    pq.push({0.0, start});

    while (!pq.empty()) {
        const auto [currentDistance, node] = pq.top();
        pq.pop();

        if (currentDistance > dist[node]) {
            continue;
        }

        for (const auto& edge : graph.getNeighbors(node)) {
            const double newDistance = currentDistance + edge.weight;

            if (newDistance < dist[edge.to]) {
                dist[edge.to] = newDistance;
                prev[edge.to] = node;
                pq.push({newDistance, edge.to});
            }
        }
    }

    return dist;
}

std::vector<std::string> RoutePlanner::reconstructPath(
    const std::unordered_map<std::string,std::string>& prev,
    const std::string& start,
    const std::string& goal) const
{
    if (!graph.hasNode(start) || !graph.hasNode(goal)) {
        return {};
    }

    std::vector<std::string> path;
    std::string step = goal;
    path.push_back(step);

    while (step != start) {
        const auto it = prev.find(step);
        if (it == prev.end()) {
            return {};
        }

        step = it->second;
        path.push_back(step);
    }

    std::reverse(path.begin(), path.end());
    return path;
}

std::vector<std::string> RoutePlanner::route(const std::string& start,
                                             const std::string& goal)
{
    if (!graph.hasNode(start) || !graph.hasNode(goal)) {
        return {};
    }

    if (start == goal) {
        return {start};
    }

    std::unordered_map<std::string,std::string> prev;
    const auto dist = dijkstra(start, prev);

    const auto goalDistance = dist.find(goal);
    if (goalDistance == dist.end() || !std::isfinite(goalDistance->second)) {
        return {};
    }

    return reconstructPath(prev, start, goal);
}

std::vector<std::string> RoutePlanner::routeToNearestFreeSpot(
    const std::string& start,
    std::string& chosenSpot,
    const std::unordered_map<std::string,bool>& freeSpots)
{
    chosenSpot.clear();

    if (!graph.hasNode(start)) {
        return {};
    }

    std::unordered_map<std::string,std::string> prev;
    const auto dist = dijkstra(start, prev);

    double best = kInfinity;
    std::string bestNode;

    for (const auto& entry : freeSpots) {
        if (!entry.second || !graph.hasNode(entry.first)) {
            continue;
        }

        const auto it = dist.find(entry.first);
        if (it == dist.end() || !std::isfinite(it->second)) {
            continue;
        }

        if (it->second < best ||
            (std::abs(it->second - best) <= 1e-9 &&
             (bestNode.empty() || entry.first < bestNode))) {
            best = it->second;
            bestNode = entry.first;
        }
    }

    if (bestNode.empty()) {
        return {};
    }

    chosenSpot = bestNode;
    return reconstructPath(prev, start, bestNode);
}

bool RoutePlanner::nodeCoordinate(const std::string& id, Coordinate& coordinate) const
{
    if (!graph.hasNode(id)) {
        return false;
    }

    coordinate = graph.getNode(id).coord;
    return true;
}

double RoutePlanner::pathDistance(const std::vector<std::string>& path) const
{
    if (path.size() < 2) {
        return 0.0;
    }

    double total = 0.0;
    for (std::size_t i = 1; i < path.size(); ++i) {
        if (!graph.hasNode(path[i - 1]) || !graph.hasNode(path[i])) {
            return kInfinity;
        }

        total += graph.getNode(path[i - 1]).coord.distanceTo(graph.getNode(path[i]).coord);
    }

    return total;
}
