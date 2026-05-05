#include <gtest/gtest.h>

#include <string>
#include <unordered_map>
#include <vector>

#include "Gate.hpp"
#include "ParkingLot.hpp"
#include "ParkingModel.hpp"
#include "ParkingSpot.hpp"
#include "coordinate.h"
#include "graph.h"
#include "routeplanner.h"

// =======================================================
// ParkingLot tests
// =======================================================

TEST(ParkingLotTest, EmptyLotHasZeroStats)
{
    ParkingLot lot("lotA", "Lot A");

    EXPECT_EQ(lot.getTotalSpots(), 0);
    EXPECT_EQ(lot.getFreeSpots(), 0);
    EXPECT_DOUBLE_EQ(lot.getOccupancyPercent(), 0.0);
}

TEST(ParkingLotTest, CountsFreeAndOccupiedSpots)
{
    ParkingLot lot("lotA", "Lot A");

    lot.addSpot(ParkingSpot("A1", "lotA", 0, 0, false)); // free
    lot.addSpot(ParkingSpot("A2", "lotA", 1, 0, true));  // occupied
    lot.addSpot(ParkingSpot("A3", "lotA", 2, 0, false)); // free

    EXPECT_EQ(lot.getTotalSpots(), 3);
    EXPECT_EQ(lot.getFreeSpots(), 2);
    EXPECT_NEAR(lot.getOccupancyPercent(), 33.3333333333, 0.0001);
}

TEST(ParkingLotTest, CanFindExistingSpotById)
{
    ParkingLot lot("lotA", "Lot A");
    lot.addSpot(ParkingSpot("A1", "lotA", 0, 0, false));

    ParkingSpot* spot = lot.getSpot("A1");

    ASSERT_NE(spot, nullptr);
    EXPECT_EQ(spot->getId(), "A1");
    EXPECT_EQ(spot->getLotId(), "lotA");
    EXPECT_FALSE(spot->isOccupied());
}

// =======================================================
// ParkingModel tests
// =======================================================

TEST(ParkingModelTest, CanAddAndFindLot)
{
    ParkingModel model;
    ParkingLot lot("lotA", "Lot A");

    model.addLot(lot);

    const ParkingLot* found = model.getLot("lotA");

    ASSERT_NE(found, nullptr);
    EXPECT_EQ(found->getId(), "lotA");
    EXPECT_EQ(found->getName(), "Lot A");
}

TEST(ParkingModelTest, CanUpdateSpotStatus)
{
    ParkingModel model;

    ParkingLot lot("lotA", "Lot A");
    lot.addSpot(ParkingSpot("A1", "lotA", 0, 0, false));
    model.addLot(lot);

    model.updateSpot("lotA", "A1", true);

    ParkingSpot* spot = model.getSpot("lotA", "A1");

    ASSERT_NE(spot, nullptr);
    EXPECT_TRUE(spot->isOccupied());
}

TEST(ParkingModelTest, UpdatingMissingSpotDoesNotCrashOrCreateSpot)
{
    ParkingModel model;

    ParkingLot lot("lotA", "Lot A");
    model.addLot(lot);

    EXPECT_NO_THROW(model.updateSpot("lotA", "missing", true));
    EXPECT_EQ(model.getSpot("lotA", "missing"), nullptr);
}

// =======================================================
// RoutePlanner tests
// =======================================================

TEST(RoutePlannerTest, ReturnsEmptyRouteForInvalidStart)
{
    ParkingLot lot("lotA", "Lot A");

    lot.addGate(Gate("G1", "lotA", -1, 0));
    lot.addSpot(ParkingSpot("A1", "lotA", 0, 0, false));

    RoutePlanner planner(lot);

    std::vector<std::string> route = planner.route("bad_start", "A1");

    EXPECT_TRUE(route.empty());
}

TEST(RoutePlannerTest, RouteFromNodeToItselfHasOneNode)
{
    ParkingLot lot("lotA", "Lot A");

    lot.addGate(Gate("G1", "lotA", -1, 0));
    lot.addSpot(ParkingSpot("A1", "lotA", 0, 0, false));

    RoutePlanner planner(lot);

    std::vector<std::string> route = planner.route("G1", "G1");

    ASSERT_EQ(route.size(), 1);
    EXPECT_EQ(route[0], "G1");
}

TEST(RoutePlannerTest, FindsRouteToNearestFreeSpot)
{
    ParkingLot lot("lotA", "Lot A");

    lot.addGate(Gate("G1", "lotA", -1, 0));
    lot.addSpot(ParkingSpot("A1", "lotA", 0, 0, false));

    RoutePlanner planner(lot);

    std::unordered_map<std::string, bool> freeSpots;
    freeSpots["A1"] = true;

    std::string chosenSpot;
    std::vector<std::string> route =
        planner.routeToNearestFreeSpot("G1", chosenSpot, freeSpots);

    EXPECT_EQ(chosenSpot, "A1");

    ASSERT_FALSE(route.empty());
    EXPECT_EQ(route.front(), "G1");
    EXPECT_EQ(route.back(), "A1");
}

TEST(RoutePlannerTest, DoesNotChooseOccupiedSpot)
{
    ParkingLot lot("lotA", "Lot A");

    lot.addGate(Gate("G1", "lotA", -1, 0));

    lot.addSpot(ParkingSpot("A1", "lotA", 0, 0, true));   // occupied
    lot.addSpot(ParkingSpot("A2", "lotA", 2, 0, false));  // free

    RoutePlanner planner(lot);

    std::unordered_map<std::string, bool> freeSpots;
    freeSpots["A1"] = false;
    freeSpots["A2"] = true;

    std::string chosenSpot;
    std::vector<std::string> route =
        planner.routeToNearestFreeSpot("G1", chosenSpot, freeSpots);

    EXPECT_EQ(chosenSpot, "A2");

    ASSERT_FALSE(route.empty());
    EXPECT_EQ(route.front(), "G1");
    EXPECT_EQ(route.back(), "A2");
}
