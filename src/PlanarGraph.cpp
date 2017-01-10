#include "PlanarGraph.h"

#include <iostream>

using namespace std;

std::shared_ptr<Graph>
PlanarGraph::createSimilar() const {
  auto graph = std::make_shared<PlanarGraph>(getId());
  graph->setNodeVisibility(getNodeVisibility());
  graph->setEdgeVisibility(getEdgeVisibility());
  graph->setFaceVisibility(getFaceVisibility());
  graph->setLabelVisibility(getLabelVisibility());
  graph->setLineWidth(getLineWidth());
  graph->setNodeArray(nodes);
  
  return graph;
}

bool
PlanarGraph::checkConsistency() const {
  return getNodeArray().size() - getEdgeCount() + getFaceCount() == 2;
}

void
PlanarGraph::mapFacesToNodes(Graph & target) {
#if 0
  assert(!target.getNodeCount() && !target.getEdgeCount() && !target.getFaceCount());
  for (unsigned int i = 0; i < getFaceCount(); i++) {
    int node_id = target.addNode();
    int region = getFaceRegion(i);
    target.setNodeColor2(i, getRegionColor(region));
  }
  unsigned int added_edge_count = 0;
  bool has_arcs = getNodeArray().hasArcData();
  for (unsigned int i = 0; i < getFaceCount(); i++) {
    int edge = getFaceFirstEdge(i);
    glm::vec3 center;
    unsigned int n = 0;
    while (edge != -1) {
      if (has_arcs && 0) {

      } else {
	pair<int, int> ed = getFaceEdgeSourceNodeAndDirection(i, edge);
	center += getNodeArray().getPosition(ed.first);
	n++;
      }
      int other_face = getEdgeTargetFace(i, edge);
      if (other_face == i) {
	cerr << "skipping self edge\n";
      } else if (other_face != -1) {
	target.addEdge(i, other_face);
	added_edge_count++;
      }
      edge = getFaceNextEdge(i, edge);
    }
    if (!n) {
      cerr << "graph has no nodes\n";
    } else {
      center.x /= n;
      center.y /= n;
      center.z /= n;
      target.setPosition(i, center);
    }
  }

  cerr << "added edges " << added_edge_count << endl;

  auto & columns = getFaceData().getColumns();
  for (auto it = columns.begin(); it != columns.end(); it++) {
    if (it->first[0] != '_') {
      cerr << "adding column " << it->first << endl;
      target.getNodeArray().getTable().addColumn(it->second);
    }
  }
#endif
}

void
PlanarGraph::mapRegionsToNodes(Graph & target) {
#if 0
  assert(!target.getNodeCount() && !target.getEdgeCount() && !target.getFaceCount());
  for (unsigned int i = 0; i < getFaceCount(); i++) {
    int node_id = target.addNode();
    int region = getFaceRegion(i);
    target.setNodeColor2(i, getRegionColor(region));
  }
  unsigned int added_edge_count = 0;
  bool has_arcs = getNodeArray().hasArcData();
  for (unsigned int i = 0; i < getFaceCount(); i++) {
    int edge = getFaceFirstEdge(i);
    glm::vec3 center;
    unsigned int n = 0;
    while (edge != -1) {
      if (has_arcs && 0) {

      } else {
	pair<int, int> ed = getFaceEdgeSourceNodeAndDirection(i, edge);
	center += getNodeArray().getPosition(ed.first);
	n++;
      }
      int other_face = getEdgeTargetFace(i, edge);
      if (other_face == i) {
	cerr << "skipping self edge\n";
      } else if (other_face != -1) {
	target.addEdge(i, other_face);
	added_edge_count++;
      }
      edge = getFaceNextEdge(i, edge);
    }
    if (!n) {
      cerr << "graph has no nodes\n";
    } else {
      center.x /= n;
      center.y /= n;
      center.z /= n;
      target.setPosition(i, center);
    }
  }

  cerr << "added edges " << added_edge_count << endl;

  auto & columns = getFaceData().getColumns();
  for (auto it = columns.begin(); it != columns.end(); it++) {
    if (it->first[0] != '_') {
      cerr << "adding column " << it->first << endl;
      target.getNodeArray().getTable().addColumn(it->second);
    }
  }
#endif
}

void
PlanarGraph::calculateRegionAreas() {
#if 0
  auto & areas = regions.addDoubleColumn("Area");
  auto & g = dynamic_cast<ColumnArc &>(edges["_geometry"]);

  bool has_arcs = getNodeArray().hasArcData();
  assert(has_arcs);

  for (unsigned int region = 0; region < getRegionCount(); region++) {
    double area = 0;
    int face = getRegionFirstFace(region);
    while (face != -1) {
      int edge = getFaceFirstEdge(face);
      while (edge != -1) {
	pair<int, int> ed = getFaceEdgeSourceNodeAndDirection(face, edge);
	if (has_arcs) {
	  const ArcData2D & arc = g.getArc(edge);
	  for (unsigned int k = 0; k + 1 < arc.size(); k++) {
	    const glm::dvec3 & v1 = arc.geometry[ed.second == -1 ? arc.size() - 1 - k : k];
	    const glm::dvec3 & v2 = arc.geometry[ed.second == -1 ? arc.size() - 1 - (k + 1) : k + 1];
	    area -= v1.x * v2.y - v1.y * v2.x;
	  }
	} else {
	  assert(0);
	}
	edge = getFaceNextEdge(face, edge);
      }
      face = getRegionNextFace(region, face);
    }
    areas.setValue(region, area);
  }
#endif
}

void
PlanarGraph::colorizeRegions() {
#if 0
  vector<glm::vec3> colors;
  colors.push_back(glm::vec3(150.0f / 255.0f, 180.0f / 255.0f, 0.0f));
  colors.push_back(glm::vec3(245.0f / 255.0f, 90.0f / 255.0f, 40.0f / 255.0f));
  colors.push_back(glm::vec3(0.0f / 255.0f, 180.0f / 255.0f, 200.0f / 255.0f));
  colors.push_back(glm::vec3(110.0f / 255.0f, 80.0f / 255.0f, 150.0f / 255.0f));
  colors.push_back(glm::vec3(1.0f, 180.0f / 255.0f, 0.0f));
  colors.push_back(glm::vec3(230.0f / 255.0f, 0.0f, 130.0f / 255.0f));
  colors.push_back(glm::vec3(70.0f / 255.0f, 70.0f / 255.0f, 50.0f / 255.0f));
  
  vector<int> region_colors(getRegionCount(), 0);
  assert(region_colors.size() == getRegionCount());

  for (unsigned int i = 0; i < getRegionCount(); i++) {
#if 0
    int edge = getRegionFirstEdge(i);
    set<int> available_colors;
    for (unsigned int j = 0; j < colors.size(); j++) {
      available_colors.insert(j);
    }
    while (edge != -1) {
      int other_face = getEdgeTargetFace(i, edge);
      if (other_face >= 0 && other_face < i) {
	available_colors.erase(face_colors[i]);
      }
      edge = getFaceNextEdge(i, edge);
    }
    set<int>::iterator it = available_colors.begin();
    assert(it != available_colors.end());
    face_colors[i] = *it;
    face_attributes[i].color = colors[*it];
#endif
  }
#endif
}

static inline double determinant(double x1, double y1, double x2, double y2) {
  return x1 * y2 - x2 * y1;
}

/**
 * returns vector cross product of vectors p1p2 and p1p3 using Cramer's rule
 */
static inline double crossProduct(const glm::dvec3 & p1, const glm::dvec3 & p2, const glm::dvec3 & p3) {
  double det_p2p3 = determinant(p2.x, p2.y, p3.x, p3.y);
  double det_p1p3 = determinant(p1.x, p1.y, p3.x, p3.y);
  double det_p1p2 = determinant(p1.x, p1.y, p2.x, p2.y);
  return det_p2p3 - det_p1p3 + det_p1p2;
}

int
PlanarGraph::findContainingRegion(const glm::dvec3 & point) const {
#if 0
  for (unsigned int i = 0; i < getFaceCount(); i++) {
    if (1 || face_attributes[i].mbr.contains(point.x, point.y)) {
      list<pair<int, int> > edges;
      int edge = getFaceFirstEdge(i);
      while (edge != -1) {
	pair<int, int> ed = getFaceEdgeSourceNodeAndDirection(i, edge);
	assert(ed.first != -1);
	edges.push_front(pair<int, int>(edge, ed.second));
	edge = getFaceNextEdge(i, edge);
	assert(edge >= -1 && edge < (int)getEdgeCount());
      }
      assert(!edges.empty());
    
      int wn = 0; // the winding number counter
  
      auto & g = dynamic_cast<const ColumnArc &>(getEdgeData()["_geometry"]);
      glm::dvec3 v1;
      for (list<pair<int, int> >::iterator it = edges.begin(); it != edges.end(); it++) {
	const ArcData2D & arc = g.getArc(it->first);
	for (unsigned int i = 0; i < arc.size(); i++) {
	  const glm::dvec3 & v2 = arc.geometry[it->second == -1 ? arc.size() - 1 - i : i];
	  if (!(v1.x == 0 && v1.y == 0 && v1.z == 0)) {
	    if (v1.y <= point.y) { // start y <= P.y
	      if (v2.y > point.y) { // an upward crossing
		if (crossProduct(v1, v2, point) > 0) {
		  // point left of edge
		  wn++; // have a valid up intersect
		}
	      }
	    } else { // start y > P.y (no test needed)
	      if (v2.y <= point.y) { // a downward crossing
		if (crossProduct(v1, v2, point) < 0) {
		  // point right of edge
		  wn--; // have a valid down intersect
		}
	      }
	    }
	  }
	  v1 = v2;
	}
      }
      if (wn != 0) {
	return getFaceRegion(i);
      }
    }
  }
#endif
  return -1;
}

void
PlanarGraph::spatialAggregation(const Graph & other) {
#if 0
  auto & columns = other.getNodeArray().getTable().getColumns();
  for (auto it = columns.begin(); it != columns.end(); it++) {
    if (it->first == "COUNT" || it->first == "LIKES") {
      cerr << "adding column " << it->first << endl;
      getRegionData().addIntColumn(it->first.c_str());
    }
  }

  for (unsigned int i = 0; i < other.getNodeArray().size(); i++) {
    const glm::vec3 & v = other.getNodeArray().getPosition(i);
    int region = findContainingRegion(glm::dvec3(v.x, v.y, v.z));
    if (region != -1) {
      for (auto it = columns.begin(); it != columns.end(); it++) {
	const string & n = it->first;
	if (n == "COUNT" || n == "LIKES") {
	  auto & col = getRegionData()[n];
	  col.setValue(region, col.getInt(region) + it->second->getInt(i));
	}
      }
    }
  }
#endif
}

set<int>
PlanarGraph::getAdjacentRegions() const {
  set<int> r;

  return r;
}

#if 0
void
PlanarGraph::addUniversalRegion() {
  int region = addRegion();
  int face = addFace(region);
  region_attributes[region].label = "UNIVERSAL";
}
#endif
