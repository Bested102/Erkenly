#include "graph.h"

void Graph::addNode(const std::string& id, const Coordinate& coord)
{
    nodes[id] = {id, coord};
}

void Graph::addEdge(const std::string& from, const std::string& to, double weight)
{
    adjacency[from].push_back({to, weight});
}

const std::unordered_map<std::string, Node>& Graph::getNodes() const
{
    return nodes;
}

const std::vector<Edge>& Graph::getNeighbors(const std::string& id) const
{
    static std::vector<Edge> empty;

    auto it = adjacency.find(id);

    if (it == adjacency.end())
        return empty;

    return it->second;
}

bool Graph::hasNode(const std::string& id) const
{
    return nodes.find(id) != nodes.end();
}

const Node& Graph::getNode(const std::string& id) const
{
    return nodes.at(id);
}
