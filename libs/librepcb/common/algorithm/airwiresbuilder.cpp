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
#include "airwiresbuilder.h"

#include <unordered_map>

#include <QtCore>

/*******************************************************************************
 *  Namespace
 ******************************************************************************/
namespace librepcb {

/*******************************************************************************
 *  Constructors / Destructor
 ******************************************************************************/

AirWiresBuilder::AirWiresBuilder() noexcept {
}

AirWiresBuilder::~AirWiresBuilder() noexcept {
}

/*******************************************************************************
 *  Public Methods
 ******************************************************************************/

int AirWiresBuilder::addPoint(const Point& p) noexcept {
  int id = mPoints.size();
  mPoints.emplace_back(p.getX().toNm(), p.getY().toNm(), id);
  return id;
}

void AirWiresBuilder::addEdge(int p1, int p2) noexcept {
  mEdges.emplace_back(mPoints[p1], mPoints[p2], -1);
}

AirWiresBuilder::AirWires AirWiresBuilder::buildAirWires() noexcept {
  // remember how many edges are already known as connected
  uint connectedEdges = mEdges.size();

  // determine additional edges between found points (candidates for airwires)
  if (mPoints.size() <= 1) {
    // no triangulation needed
  } else if (mPoints.size() == 2) {
    // manual triangulation since delaunay doesn't work properly in this case
    mEdges.emplace_back(mPoints[0], mPoints[1], -1);
  } else if (arePointsColinear()) {
    // manual triangulation since delaunay doesn't work properly in this case
    std::vector<delaunay::Vector2<qreal>> sortedPoints = mPoints;
    std::sort(sortedPoints.begin(), sortedPoints.end(),
              [](const delaunay::Vector2<qreal>& a,
                 const delaunay::Vector2<qreal>& b) { return a.x < b.x; });
    for (size_t i = 1; i < sortedPoints.size(); i++) {
      mEdges.emplace_back(sortedPoints[i - 1], sortedPoints[i], -1);
    }
  } else {
    // triangulate with delaunay
    delaunay::Delaunay<qreal> del;
    del.triangulate(mPoints);
    mEdges.insert(mEdges.end(), del.getEdges().begin(), del.getEdges().end());
  }

  // determine weights of these new edges
  for (uint i = connectedEdges; i < mEdges.size(); ++i) {
    mEdges[i].weight = mEdges[i].p1.dist2(mEdges[i].p2);
  }

  // find airwires in list of edges
  return AirWiresBuilder::kruskalMst();
}

/*******************************************************************************
 *  Private Methods
 ******************************************************************************/

// copied from horizon
bool AirWiresBuilder::arePointsColinear() const noexcept {
  if (mPoints.size() < 3) return true;
  auto        it  = mPoints.cbegin();
  const auto& pp0 = *it++;
  QPointF     p0(pp0.x, pp0.y);
  const auto& pp1 = *it++;
  QPointF     p1(pp1.x, pp1.y);
  auto        v  = p1 - p0;
  auto        a0 = std::atan2(v.y(), v.x());
  if (a0 < 0) a0 += M_PI;
  for (; it != mPoints.cend(); it++) {
    QPointF p(it->x, it->y);
    auto    vp = p - p0;
    if ((vp.x() * vp.x() + vp.y() * vp.y()) > 1e6) {  // > 1µm
      auto a = std::atan2(vp.y(), vp.x());
      if (a < 0) a += M_PI;
      if (std::abs(a - a0) > 1e-6) {
        return false;
      }
    }
  }
  return true;
}

// adapted from horizon/kicad
AirWiresBuilder::AirWires AirWiresBuilder::kruskalMst() noexcept {
  unsigned int nodeNumber      = mPoints.size();
  unsigned int mstExpectedSize = nodeNumber - 1;
  unsigned int mstSize         = 0;
  bool         ratsnestLines   = false;

  // printf("mst nodes : %d edges : %d\n", mPoints.size(), mEdges.size () );
  // The output
  AirWires mst;

  // Set tags for marking cycles
  std::unordered_map<int, int> tags;
  unsigned int                 tag = 0;

  for (auto& node : mPoints) {
    node.tag      = tag;
    tags[node.id] = tag++;
  }

  // Lists of nodes connected together (subtrees) to detect cycles in the
  // graph
  std::vector<std::list<int>> cycles(nodeNumber);

  for (unsigned int i = 0; i < nodeNumber; ++i) cycles[i].push_back(i);

  // Kruskal algorithm requires edges to be sorted by their weight
  std::sort(mEdges.begin(), mEdges.end(),
            [](const delaunay::Edge<qreal>& a, const delaunay::Edge<qreal>& b) {
              return a.weight > b.weight;
            });

  while (mstSize < mstExpectedSize && !mEdges.empty()) {
    auto& dt = mEdges.back();

    int srcTag = tags[dt.p1.id];
    int trgTag = tags[dt.p2.id];
    // printf("mstSize %d %d %f %d<>%d\n", mstSize, mstExpectedSize,
    // dt.weight, srcTag, trgTag);

    // Check if by adding this edge we are going to join two different
    // forests
    if (srcTag != trgTag) {
      // Because edges are sorted by their weight, first we always process
      // connected
      // items (weight == 0). Once we stumble upon an edge with non-zero
      // weight,
      // it means that the rest of the lines are ratsnest.
      if (!ratsnestLines && dt.weight >= 0) ratsnestLines = true;

      // Update tags
      if (ratsnestLines) {
        for (auto it = cycles[trgTag].begin(); it != cycles[trgTag].end();
             ++it) {
          tags[mPoints[*it].id] = srcTag;
        }

        // Do a copy of edge, but make it RN_EDGE_MST. In contrary to
        // RN_EDGE,
        // RN_EDGE_MST saves both source and target node and does not
        // require any other
        // edges to exist for getting source/target nodes
        // CN_EDGE newEdge ( dt.GetSourceNode(), dt.GetTargetNode(),
        // dt.GetWeight() );

        // assert( newEdge.GetSourceNode()->GetTag() !=
        // newEdge.GetTargetNode()->GetTag() );
        // assert(dt.p1.tag != dt.p2.tag);
        mst.append(qMakePair(Point(dt.p1.x, dt.p1.y), Point(dt.p2.x, dt.p2.y)));
        ++mstSize;
      } else {
        // for( it = cycles[trgTag].begin(), itEnd =
        // cycles[trgTag].end(); it != itEnd; ++it )
        // for( auto it : cycles[trgTag] )
        for (auto it = cycles[trgTag].begin(); it != cycles[trgTag].end();
             ++it) {
          tags[mPoints[*it].id] = srcTag;
          mPoints[*it].tag      = srcTag;
        }

        // Processing a connection, decrease the expected size of the
        // ratsnest MST
        --mstExpectedSize;
      }

      // Move nodes that were marked with old tag to the list marked with
      // the new tag
      cycles[srcTag].splice(cycles[srcTag].end(), cycles[trgTag]);
    }

    // Remove the edge that was just processed
    mEdges.pop_back();
  }

  return mst;
}

/*******************************************************************************
 *  End of File
 ******************************************************************************/

}  // namespace librepcb
