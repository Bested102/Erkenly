#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "coordinate.h"

struct Edge
{
    std::string to;
    double weight;
};

struct Node
{
    std::string id;
    Coordinate coord;
};

class Graph
{
private:

    std::unordered_map<std::string, Node> nodes;

    std::unordered_map<std::string, std::vector<Edge>> adjacency;

public:

    void addNode(const std::string& id, const Coordinate& coord);

    void addEdge(const std::string& from, const std::string& to, double weight);

    const std::unordered_map<std::string, Node>& getNodes() const;

    const std::vector<Edge>& getNeighbors(const std::string& id) const;

    bool hasNode(const std::string& id) const;

    const Node& getNode(const std::string& id) const;
};
