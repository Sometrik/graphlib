#include "PlanarGraph.h"

#include <iostream>

using namespace std;

PlanarGraph::PlanarGraph(int _id) : Graph(2, _id) {
  
}

PlanarGraph::PlanarGraph(const PlanarGraph & other)
  : Graph(other), edge_attributes(other.edge_attributes) {

}

std::shared_ptr<Graph>
PlanarGraph::createSimilar() const {
  std::shared_ptr<Graph> graph(new PlanarGraph(getId()));
  graph->setLocationGraphValid(false);
  graph->setTemporal(isTemporal());
  graph->setPersonality(getPersonality());
  graph->setHasTextures(hasTextures());
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
  bool has_arcs = hasArcData();
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
  bool has_arcs = hasArcData();
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

  bool has_arcs = hasArcData();
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
}

void
PlanarGraph::updateMBR(int edge) {
  if (hasArcData()) {
#if 0
    auto & g = dynamic_cast<ColumnArc&>(getEdgeData()["_geometry"]);
    auto & arc = g.getArc(edge);
    
    int left_face = edges["_leftFace"].getInt(edge);
    int right_face = edges["_rightFace"].getInt(edge);
    if (left_face != -1) {
      Rect2d & face_mbr = face_attributes[left_face].mbr;
      Rect2d & region_mbr = region_attributes[getFaceRegion(left_face)].mbr;
      for (vector<glm::dvec3>::const_iterator it = arc.geometry.begin(); it != arc.geometry.end(); it++) {
	face_mbr.growToContain(glm::vec3(it->x, it->y, it->z));
	region_mbr.growToContain(glm::vec3(it->x, it->y, it->z));
      }
    }
    if (right_face != -1) {
      Rect2d & face_mbr = face_attributes[right_face].mbr;
      Rect2d & region_mbr = region_attributes[getFaceRegion(right_face)].mbr;
      for (vector<glm::dvec3>::const_iterator it = arc.geometry.begin(); it != arc.geometry.end(); it++) {
	face_mbr.growToContain(glm::vec3(it->x, it->y, it->z));
	region_mbr.growToContain(glm::vec3(it->x, it->y, it->z));
      }
    }
#endif
  }
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
}

set<int>
PlanarGraph::getAdjacentRegions() const {
  set<int> r;

  return r;
}

void
PlanarGraph::addUniversalRegion() {
  int region = addRegion();
  int face = addFace(region);
  region_attributes[region].label = "UNIVERSAL";  
}

int
PlanarGraph::addEdge(int n1, int n2, int face, float weight, int arc, long long coverage) {
  assert(n1 != -1 && n2 != -1);
  int edge = (int)edge_attributes.size();
  int next_node_edge = getNodeFirstEdge(n1);
  setNodeFirstEdge(n1, edge);
  if (n1 != n2) {
    updateOutdegree(n1, 1);
    updateIndegree(n2, 1);
  }
  updateNodeSize(n1);
  updateNodeSize(n2);
  
  edge_attributes.push_back(planar_edge_data_s( weight, n1, n2, next_node_edge, -1, -1, arc ));
  edges.addRow();
  total_edge_weight += fabsf(weight);
  
  setEdgeFace(edge, face);
  
  return edge;
}
