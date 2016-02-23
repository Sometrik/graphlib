#include "ShapefileLoader.h"

#include "PlanarGraph.h"
#include "ArcData2D.h"

#include <Table.h>
#include <DBase3File.h>

#include <cassert>
#include <shapefil.h>
#include <iostream>
#include <cmath>
#include <sstream>

using namespace std;

ShapefileLoader::ShapefileLoader() : FileTypeHandler("ESRI Shapefile", false) {
  addExtension("shp");
}

std::shared_ptr<Graph>
ShapefileLoader::openGraph(const char * filename, const std::shared_ptr<NodeArray> & initial_nodes) {
  SHPHandle shp = SHPOpen(filename, "rb");
  if (!shp) {
    cerr << "failed to open shapefile\n";
    return 0;
  }

  double minb[4], maxb[4];
  int shape_count, global_shape_type;
  SHPGetInfo(shp, &shape_count, &global_shape_type, minb, maxb);
  
  std::shared_ptr<Graph> graph;

  map<string, int> nodes;
  map<string, pair<glm::dvec2, set<string> > > node_edges;
  map<string, int> waiting_faces;

  cerr << "loading shapefile (" << shape_count << ")\n";

  bool has_polygons = false;

  for (int i = 0; i < shape_count; i++) {
    SHPObject * o = SHPReadObject(shp, i);
    assert(o);
    
    switch (o->nSHPType) {
    case SHPT_POINT:
    case SHPT_POINTZ:
    case SHPT_POINTM:
      if (!graph.get()) {
	graph = std::make_shared<PlanarGraph>();
	graph->setNodeArray(initial_nodes);
	graph->setHasSpatialData(true);
	graph->getNodeArray().setNodeVisibility(false);
      }
      assert(o->nVertices == 1);
      {
	int face_id = graph->addFace();
	double x = o->padfX[0], y = o->padfY[0], z = o->padfZ[0];
	int node_id = createNode2D(*graph, nodes, x, y);
	graph->addEdge(node_id, node_id, face_id, 0.0f);
	auto & fd = graph->getFaceAttributes(face_id);
	fd.centroid = glm::vec2(x, y);
	fd.mbr = glm::vec3(x, y, 0.0f);
      }
      break;
    case SHPT_MULTIPOINT:
    case SHPT_MULTIPOINTZ:
    case SHPT_MULTIPOINTM:
      assert(0);
#if 0
      if (!graph.get()) {
	graph = std::make_shared<PlanarGraph>();
	graph->setNodeArray(initial_nodes);
	graph->setHasSpatialData(true);
      }
      for (int j = 0; j < o->nVertices; j++) {
	double x = o->padfX[j], y = o->padfY[j], z = o->padfZ[j];
	int node_id = createNode2D(*graph, nodes, x, y);
	graph->addEdge(node_id, node_id, -1, 0.0f);
      }
#endif
      break;
    case SHPT_ARC: // = polyline
    case SHPT_ARCZ:
    case SHPT_ARCM:
      if (!graph.get()) {
	graph = std::make_shared<PlanarGraph>();
	graph->setNodeArray(initial_nodes);
	graph->setHasSpatialData(true);
	graph->setHasArcData(true);
	graph->getNodeArray().setNodeVisibility(false);
	graph->getNodeArray().setFaceVisibility(false);
      }
      {
	int hyperedge_id = graph->addFace();
	for (int j = 0; j < o->nParts; j++) {
	  int start = o->nParts > 1 ? o->panPartStart[j] : 0;
	  int end = o->nParts > 1 && j + 1 < o->nParts ? o->panPartStart[j + 1] : o->nVertices;
	  ArcData2D arc;
	  for (int k0 = start; k0 < end; k0++) {
	    double x = o->padfX[k0], y = o->padfY[k0], z = o->padfZ[k0];
	    arc.data.push_back(glm::dvec2(x, y));
	  }

	  int arc_id = graph->getNodeArray().addArcGeometry(arc);
	  pair<int, int> n = createNodesForArc(arc, *graph, nodes);
	  assert(arc_id);
	  assert(arc_id >= 1 && arc_id <= graph->getNodeArray().getArcGeometry().size());
	  int edge_id = graph->addEdge(n.first, n.second, hyperedge_id, 1.0f, arc_id);
	  assert(graph->getEdgeAttributes(edge_id).arc == arc_id);
	}
      }
      break;
    case SHPT_POLYGON:
    case SHPT_POLYGONZ:
    case SHPT_POLYGONM:
      if (!graph.get()) {
	cerr << "creating planar graph for shapefile\n";
	graph = std::make_shared<PlanarGraph>();
	graph->setNodeArray(initial_nodes);
	graph->setHasSpatialData(true);
	graph->setHasArcData(true);
	graph->getNodeArray().setNodeVisibility(false);
	graph->getNodeArray().setEdgeVisibility(false);
	// graph->addUniversalRegion();
      }
      has_polygons = true;
      for (int j = 0; j < o->nParts; j++) {
	int start = o->nParts > 1 ? o->panPartStart[j] : 0;
	int end = o->nParts > 1 && j + 1 < o->nParts ? o->panPartStart[j + 1] : o->nVertices;
	int n = end - start;
	for (int k0 = 0; k0 + 1 < n; k0++) {
	  int k1 = start + (k0 % n), k2 = start + ((k0 + 1) % n);
	  glm::dvec2 v1(o->padfX[k1], o->padfY[k1]); //, o->padfZ[k]);
	  glm::dvec2 v2(o->padfX[k2], o->padfY[k2]); // o->padfZ[k + 1]);
	  ostringstream key1, key2;
	  key1 << v1.x << "/" << v1.y;
	  key2 << v2.x << "/" << v2.y;
	  pair<glm::dvec2, set<string> > & ne1 = node_edges[key1.str()];
	  pair<glm::dvec2, set<string> > & ne2 = node_edges[key2.str()];
	  ne1.first = v1;
	  ne2.first = v2;
	  ne1.second.insert(key2.str());
	  ne2.second.insert(key1.str());
	}
      }
      break;
    case SHPT_MULTIPATCH:
      cerr << "multipatches not supported\n";
      assert(0);
      break;
    default:
      cerr << "unhandled type\n";
      assert(0);
    }
    SHPDestroyObject(o);
  }

  if (has_polygons) {
    unsigned int connected_face_arcs = 0;
    cerr << "creating nodes (" << node_edges.size() << ")\n";
    for (map<string, pair<glm::dvec2, set<string> > >::iterator it = node_edges.begin(); it != node_edges.end(); it++) {
      if (it->second.second.size() >= 3) {
	createNode2D(*graph, nodes, it->second.first.x, it->second.first.y);
      }
    }
    cerr << "creating faces\n";    
    for (int i = 0; i < shape_count; i++) {
      SHPObject * o = SHPReadObject(shp, i);
      assert(o);
      assert(o->nSHPType == SHPT_POLYGON || o->nSHPType == SHPT_POLYGONZ || o->nSHPType == SHPT_POLYGONM);
      // int region_id = graph->addRegion();
      int face_id = graph->addFace();
      double area = 0, centroid_x = 0, centroid_y = 0;	
      assert(o->nParts >= 1); // could be zero to
      for (int j = 0; j < o->nParts; j++) {
	// int face_id = graph->addFace(region_id);
	// cerr << "handling polygon " << i << ", part " << j << endl;
	int start = o->panPartStart[j];
	if (j == 0 && start > 0) {
	  cerr << "invalid start " << start << endl;
	  start = 0;
	}
	int end = j + 1 < o->nParts ? o->panPartStart[j + 1] : o->nVertices;
	list<glm::dvec2> input;
	for (int k = start; k < end; k++) {
	  double x = o->padfX[k], y = o->padfY[k], z = o->padfZ[k];
	  input.push_back(glm::dvec2(x, y));
	  if (k + 1 < end) {
	    double a = o->padfX[k] * o->padfY[k + 1] - o->padfY[k] * o->padfX[k + 1];
	    area -= a;
	    centroid_x -= (o->padfX[k] + o->padfX[k + 1]) * a;
	    centroid_y -= (o->padfY[k] + o->padfY[k + 1]) * a;
	  }
	}	
	if (input.size() > 1 &&
	    input.front().x == input.back().x &&
	    input.front().y == input.back().y) {
	  input.pop_back();
	}
	// cerr << "rolling vertices (" << input.size() << ")\n";
	bool node_found = false;
	for (int step = 0; step < input.size(); step++) {
	  ostringstream key1;
	  key1 << input.front().x << "/" << input.front().y;
	  if (nodes.find(key1.str()) != nodes.end()) {
	    node_found = true;
	    break;
	  } else {
	    input.push_back(input.front());
	    input.pop_front();
	  }
	}
	input.push_back(input.front());
	assert(input.size() >= 4);
	if (!node_found) { // island
	  createNode2D(*graph, nodes, input.front().x, input.front().y);
	}
	// ArcData2D arc;
	// for (list<glm::dvec3>::iterator it = input.begin(); it != input.end(); it++) {
	//   arc.data.push_back(*it);
	// }
	// pair<int, int> n = createNodesForArc(arc, graph, nodes);
	// assert(n.first == n.second);
	// int edge_id = graph->addEdge(n.first, n.second, face_id);
	// geometries.setArc(edge_id, arc);
	vector<ArcData2D> arcs;
	vector<int> face_nodes;
	for (auto & v : input) {
	  ostringstream key1;
	  key1 << v.x << "/" << v.y;
	  map<string, int>::iterator it2 = nodes.find(key1.str());
	  if (it2 != nodes.end()) {
	    if (!arcs.empty()) {
	      arcs.back().data.push_back(v);
	    }
	    face_nodes.push_back(it2->second);
	    arcs.push_back(ArcData2D());
	  }
	  assert(!arcs.empty());
	  arcs.back().data.push_back(v);		
	}
	assert(face_nodes.front() == face_nodes.back());
	for (int l = 0; l + 1 < face_nodes.size(); l++) {
	  int node1 = face_nodes[l], node2 = face_nodes[l + 1];
	  if (node1 == node2) {
	    // cerr << "same nodes, size = " << face_nodes.size() << endl;
	  }

	  int arc_id = 0, pair_edge = -1;
	  ostringstream key1;
	  key1 << node2 << "/" << node1;
	  map<string, int>::iterator it = waiting_faces.find(key1.str());
	  if (it != waiting_faces.end()) {
	    pair_edge = it->second;
	    auto & ed = graph->getEdgeAttributes(pair_edge);
	    assert(ed.arc >= 1 && ed.arc <= graph->getNodeArray().getArcGeometry().size());
	    arc_id = -ed.arc;
	    waiting_faces.erase(it);
	    connected_face_arcs++;
	  } else {
	    arc_id = graph->getNodeArray().addArcGeometry(arcs[l]);
	    assert(arc_id >= 1 && arc_id <= graph->getNodeArray().getArcGeometry().size());
	  }

	  assert(arc_id);
	  
	  int edge_id = graph->addEdge(node1, node2, face_id, 1.0f, arc_id);
	  if (pair_edge != -1) {
	    graph->connectEdgePair(edge_id, pair_edge);
	  } else {
	    graph->updateMBR(edge_id);
	    ostringstream key2;
	    key2 << node1 << "/" << node2;
	    waiting_faces[key2.str()] = edge_id;
	  }
	  assert(graph->getEdgeAttributes(edge_id).arc == arc_id);
	}
      }

      area *= 0.5;
      centroid_x /= (6.0*area);
      centroid_y /= (6.0*area);
      graph->setFaceCentroid(face_id, glm::vec2(centroid_x, centroid_y));
	
      SHPDestroyObject(o);
    }
    cerr << "ARCS: connected face arcs = " << connected_face_arcs << ", unconnected = " << waiting_faces.size() << endl;
  }

  SHPClose(shp);

  assert(graph.get());
  
  auto dbf = std::make_shared<table::DBase3File>(filename);
  
  if (dbf->getRecordCount() != shape_count) {
    cerr << "bad number of records\n";
  }
  
  table::Table & table = graph->getFaceData();
  for (auto & col : dbf->getColumns()) {
    table.addColumn(col);
  }

  graph->updateAppearance();
  
  return graph;
}
