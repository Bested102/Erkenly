#pragma once
#include <string>

class Gate {
public:

    Gate();

    Gate(const std::string& id,
         const std::string& lotId,
         double x,
         double y,
         int graphNodeId = -1);

    std::string getId() const;
    std::string getLotId() const;

    double getX() const;
    double getY() const;

    int getGraphNodeId() const;
    void setGraphNodeId(int nodeId);

private:

    std::string id;
    std::string lotId;

    double x;
    double y;

    int graphNodeId;
};