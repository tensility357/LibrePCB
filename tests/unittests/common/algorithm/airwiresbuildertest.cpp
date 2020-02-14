/*
 * LibrePCB - Professional EDA for everyone!
 * Copyright (C) 2013 LibrePCB Developers, see AUTHORS.md for contributors.
 * https://librepcb.org/
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*******************************************************************************
 *  Includes
 ******************************************************************************/

#include <gtest/gtest.h>
#include <librepcb/common/algorithm/airwiresbuilder.h>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {
namespace tests {

/*******************************************************************************
 *  Test Class
 ******************************************************************************/

class AirWiresBuilderTest : public ::testing::Test {
protected:
  static AirWiresBuilder::AirWires sorted(
      AirWiresBuilder::AirWires airwires) noexcept {
    for (AirWiresBuilder::AirWire& airwire : airwires) {
      if (airwire.second < airwire.first) {
        std::swap(airwire.second, airwire.first);
      }
    }
    std::sort(airwires.begin(), airwires.end());
    return airwires;
  }
};

/*******************************************************************************
 *  Test Methods
 ******************************************************************************/

TEST_F(AirWiresBuilderTest, testEmpty) {
  AirWiresBuilder           builder;
  AirWiresBuilder::AirWires airwires = sorted(builder.buildAirWires());
  EXPECT_EQ(0, airwires.size());
}

TEST_F(AirWiresBuilderTest, testOnePoint) {
  AirWiresBuilder builder;
  builder.addPoint(Point(100, 200));
  AirWiresBuilder::AirWires airwires = sorted(builder.buildAirWires());
  EXPECT_EQ(0, airwires.size());
}

TEST_F(AirWiresBuilderTest, testTwoUnconnectedPoints) {
  AirWiresBuilder builder;
  builder.addPoint(Point(100, 200));
  builder.addPoint(Point(300, 400));
  AirWiresBuilder::AirWires airwires = sorted(builder.buildAirWires());
  AirWiresBuilder::AirWires expected = {{Point(100, 200), Point(300, 400)}};
  EXPECT_EQ(expected, airwires);
}

TEST_F(AirWiresBuilderTest, testTwoUnconnectedOverlappingPoints) {
  AirWiresBuilder builder;
  builder.addPoint(Point(100, 200));
  builder.addPoint(Point(100, 200));
  AirWiresBuilder::AirWires airwires = sorted(builder.buildAirWires());
  AirWiresBuilder::AirWires expected = {{Point(100, 200), Point(100, 200)}};
  EXPECT_EQ(expected, airwires);
}

TEST_F(AirWiresBuilderTest, testTwoConnectedPoints) {
  AirWiresBuilder builder;
  int             id0 = builder.addPoint(Point(100, 200));
  int             id1 = builder.addPoint(Point(300, 400));
  builder.addEdge(id0, id1);
  AirWiresBuilder::AirWires airwires = sorted(builder.buildAirWires());
  EXPECT_EQ(0, airwires.size());
}

TEST_F(AirWiresBuilderTest, testThreeUnconnectedPoints) {
  AirWiresBuilder builder;
  builder.addPoint(Point(100, 200));
  builder.addPoint(Point(300, 400));
  builder.addPoint(Point(-50, -50));
  AirWiresBuilder::AirWires airwires = sorted(builder.buildAirWires());
  AirWiresBuilder::AirWires expected = {{Point(-50, -50), Point(100, 200)},
                                        {Point(100, 200), Point(300, 400)}};
  EXPECT_EQ(expected, airwires);
}

TEST_F(AirWiresBuilderTest, testThreeUnconnectedPointsVshape) {
  AirWiresBuilder builder;
  builder.addPoint(Point(-5, 0));
  builder.addPoint(Point(10, 0));
  builder.addPoint(Point(0, -100));
  AirWiresBuilder::AirWires airwires = sorted(builder.buildAirWires());
  AirWiresBuilder::AirWires expected = {{Point(-5, 0), Point(0, -100)},
                                        {Point(-5, 0), Point(10, 0)}};
  EXPECT_EQ(expected, airwires);
}

// Test added for bug https://github.com/LibrePCB/LibrePCB/issues/588
TEST_F(AirWiresBuilderTest, testThreeUnconnectedColinearPoints) {
  AirWiresBuilder builder;
  builder.addPoint(Point(0, 0));
  builder.addPoint(Point(100, 0));
  builder.addPoint(Point(-100, 0));
  AirWiresBuilder::AirWires airwires = sorted(builder.buildAirWires());
  AirWiresBuilder::AirWires expected = {{Point(-100, 0), Point(0, 0)},
                                        {Point(0, 0), Point(100, 0)}};
  EXPECT_EQ(expected, airwires);
}

// Test added for bug https://github.com/LibrePCB/LibrePCB/issues/588
TEST_F(AirWiresBuilderTest, testThreeUnconnectedDiagonalColinearPoints) {
  AirWiresBuilder builder;
  builder.addPoint(Point(0, 0));
  builder.addPoint(Point(100, 100));
  builder.addPoint(Point(200, 200));
  AirWiresBuilder::AirWires airwires = sorted(builder.buildAirWires());
  AirWiresBuilder::AirWires expected = {{Point(0, 0), Point(100, 100)},
                                        {Point(100, 100), Point(200, 200)}};
  EXPECT_EQ(expected, airwires);
}

// Test added for bug https://github.com/LibrePCB/LibrePCB/issues/588
TEST_F(AirWiresBuilderTest, testThreeUnconnectedDiagonalColinearPoints2) {
  AirWiresBuilder builder;
  builder.addPoint(Point(71437500, 78898800));
  builder.addPoint(Point(70485000, 80010000));
  builder.addPoint(Point(72707500, 77470000));
  AirWiresBuilder::AirWires airwires = sorted(builder.buildAirWires());
  AirWiresBuilder::AirWires expected = {
      {Point(70485000, 80010000), Point(71437500, 78898800)},
      {Point(71437500, 78898800), Point(72707500, 77470000)}};
  EXPECT_EQ(expected, airwires);
}

// Test added for bug https://github.com/LibrePCB/LibrePCB/issues/588
TEST_F(AirWiresBuilderTest, testPartlyConnectedColinearPoints) {
  AirWiresBuilder builder;
  /*int id0 = */ builder.addPoint(Point(0, 0));
  int id1 = builder.addPoint(Point(10, 10));
  int id2 = builder.addPoint(Point(20, 20));
  /*int id3 = */ builder.addPoint(Point(30, 30));
  /*int id4 = */ builder.addPoint(Point(40, 40));
  /*int id5 = */ builder.addPoint(Point(50, 50));
  /*int id5 = */ builder.addPoint(Point(60, 60));
  builder.addEdge(id1, id2);
  AirWiresBuilder::AirWires airwires = sorted(builder.buildAirWires());
  AirWiresBuilder::AirWires expected = {{Point(0, 0), Point(10, 10)},
                                        {Point(20, 20), Point(30, 30)},
                                        {Point(30, 30), Point(40, 40)},
                                        {Point(40, 40), Point(50, 50)},
                                        {Point(50, 50), Point(60, 60)}};
  EXPECT_EQ(expected, airwires);
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace tests
}  // namespace librepcb
