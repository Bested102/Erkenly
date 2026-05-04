#pragma once

#include <cmath>

class Coordinate
{
public:

    double x;
    double y;

    Coordinate()
        : x(0), y(0) {}

    Coordinate(double x, double y)
        : x(x), y(y) {}

    double distanceTo(const Coordinate& other) const
    {
        double dx = x - other.x;
        double dy = y - other.y;

        return std::sqrt(dx * dx + dy * dy);
    }
};
