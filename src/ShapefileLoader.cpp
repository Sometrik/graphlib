#include "ShapefileLoader.h"

#include "PointCloud.h"
#include "MultiArcGraph.h"
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
ShapefileLoader::openGraph(const char * filename) {
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
    int shape_dimensions;
    
    switch (o->nSHPType) {
    case SHPT_POINT:
    case SHPT_POINTZ:
    case SHPT_POINTM:
    case SHPT_MULTIPOINT:
    case SHPT_MULTIPOINTZ:
    case SHPT_MULTIPOINTM:
      shape_dimensions = 0;
      if (!graph.get()) {
	cerr << "creating pointcloud for shapefile\n";
	graph = std::make_shared<PointCloud>();
	graph->setNodeArray(std::make_shared<NodeArray>());
	graph->setHasSpatialData(true);
      }
      assert(shape_dimensions == graph->getDimensions());
      for (int j = 0; j < o->nVertices; j++) {
	double x = o->padfX[j], y = o->padfY[j], z = o->padfZ[j];
	createNode2D(*graph, nodes, x, y);
      }
      break;
    case SHPT_ARC: // = polyline
    case SHPT_ARCZ:
    case SHPT_ARCM:
      shape_dimensions = 1;
      if (!graph.get()) {
	cerr << "creating undirected graph for shapefile\n";
	graph = std::make_shared<MultiArcGraph>();
	graph->setNodeArray(std::make_shared<NodeArray>());
	graph->setHasSpatialData(true);
	graph->setHasArcData(true);
	graph->getNodeArray().setNodeVisibility(false);
      }
      assert(shape_dimensions == graph->getDimensions());
      {
	int hyperedge_id = graph->addFace(-1);
	for (int j = 0; j < o->nParts; j++) {
	  int start = o->nParts > 1 ? o->panPartStart[j] : 0;
	  int end = o->nParts > 1 && j + 1 < o->nParts ? o->panPartStart[j + 1] : o->nVertices;
	  ArcData2D arc;
	  for (int k0 = start; k0 < end; k0++) {
	    double x = o->padfX[k0], y = o->padfY[k0], z = o->padfZ[k0];
	    arc.data.push_back(glm::dvec2(x, y));
	  }

	  int arc_id = graph->addArcGeometry(arc);
	  pair<int, int> n = createNodesForArc(arc, *graph, nodes);
	  assert(arc_id);
	  assert(arc_id >= 1 && arc_id <= graph->getArcGeometry().size());
	  int edge_id = graph->addEdge(n.first, n.second, hyperedge_id, 1.0f, arc_id);
	  assert(graph->getEdgeAttributes(edge_id).arc == arc_id);
	}
      }
      break;
    case SHPT_POLYGON:
    case SHPT_POLYGONZ:
    case SHPT_POLYGONM:
      shape_dimensions = 2;
      if (!graph.get()) {
	cerr << "creating planar graph for shapefile\n";
	graph = std::make_shared<PlanarGraph>();
	graph->setNodeArray(std::make_shared<NodeArray>());
	graph->setHasSpatialData(true);
	graph->setHasArcData(true);
	graph->getNodeArray().setNodeVisibility(false);
	graph->getNodeArray().setEdgeVisibility(false);
	// graph->addUniversalRegion();
      }
      assert(shape_dimensions == graph->getDimensions());
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
      int region_id = graph->addRegion();
      assert(o->nParts >= 1); // could be zero to
      for (int j = 0; j < o->nParts; j++) {
	int face_id = graph->addFace(region_id);
	// cerr << "handling polygon " << i << ", part " << j << endl;
	int start = o->panPartStart[j];
	if (j == 0 && start > 0) {
	  cerr << "invalid start " << start << endl;
	  start = 0;
	}
	int end = j + 1 < o->nParts ? o->panPartStart[j + 1] : o->nVertices;
	list<glm::dvec2> input;
      	double area = 0, centroid_x = 0, centroid_y = 0;	
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
	area *= 0.5;
	centroid_x /= (6.0*area);
	centroid_y /= (6.0*area);
	graph->setFaceCentroid(face_id, glm::vec2(centroid_x, centroid_y));
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
	  ostringstream key1, key2;
	  key1 << node1 << "/" << node2;
	  map<string, int>::iterator it = waiting_faces.find(key1.str());
	  if (it != waiting_faces.end()) {
	    int edge = it->second;
	    graph->setEdgeFace(edge, face_id);
	    graph->updateMBR(edge);
	    waiting_faces.erase(it);
	  } else {
	    key2 << node2 << "/" << node1;
	    it = waiting_faces.find(key2.str());
	    if (it != waiting_faces.end()) {
	      int edge = it->second;
	      graph->setEdgeFace(edge, face_id);
	      graph->updateMBR(edge);
	      waiting_faces.erase(it);
	    } else {
	      int arc_id = graph->addArcGeometry(arcs[l]);
	      assert(arc_id);
	      assert(arc_id >= 1 && arc_id <= graph->getArcGeometry().size());
	      int edge_id = graph->addEdge(node1, node2, face_id, 1.0f, arc_id);
	      // cerr << "created edge: id = " << edge_id << ", arc = " << arc_id << ", arc2 = " << graph->getEdgeAttributes(edge_id).arc << endl;
	      assert(graph->getEdgeAttributes(edge_id).arc == arc_id);
	      // waiting_faces[key1.str()] = edge_id;
	      // geometries.setArc(edge_id, arcs[l]);
	      graph->updateMBR(edge_id);
	    }
	  }
	}
      }
      SHPDestroyObject(o);
    }
  }

  SHPClose(shp);

  assert(graph.get());
  
  auto dbf = std::make_shared<table::DBase3File>(filename);
  
  if (dbf->getRecordCount() != shape_count) {
    cerr << "bad number of records\n";
  }
  
  if (graph->getDimensions() == 0) {
    table::Table & nodes_table = graph->getNodeArray().getTable();
    for (auto & col : dbf->getColumns()) {
      nodes_table.addColumn(col);
    }
  } else if (graph->getDimensions() == 1) {
    table::Table & hyperedges_table = graph->getFaceData();
    for (auto & col : dbf->getColumns()) {
      hyperedges_table.addColumn(col);
    }
  } else if (graph->getDimensions() == 2) {
    table::Table & regions_table = graph->getRegionData();
    for (auto & col : dbf->getColumns()) {
      regions_table.addColumn(col);
    }
  } else {
    assert(0);
  }
  
  return graph;
}
