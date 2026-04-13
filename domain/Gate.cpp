#include "Gate.hpp"

Gate::Gate()
    : x(0), y(0), graphNodeId(-1)
{}

Gate::Gate(const std::string& id,
           const std::string& lotId,
           double x,
           double y,
           int graphNodeId)
    : id(id),
      lotId(lotId),
      x(x),
      y(y),
      graphNodeId(graphNodeId)
{}

std::string Gate::getId() const {
    return id;
}

std::string Gate::getLotId() const {
    return lotId;
}

double Gate::getX() const {
    return x;
}

double Gate::getY() const {
    return y;
}

int Gate::getGraphNodeId() const {
    return graphNodeId;
}

void Gate::setGraphNodeId(int nodeId) {
    graphNodeId = nodeId;
}