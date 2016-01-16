#include "TopoJSONLoader.h"

#include "PlanarGraph.h"
#include "ArcData2D.h"

#include <Table.h>
#include <SRID.h>

#include <sstream>
#include <cassert>
#include <iostream>
#include <cmath>

#include <glm/glm.hpp>

using namespace std;
using namespace table;

TopoJSONLoader::TopoJSONLoader() : FileTypeHandler("TopoJSON", false) {
  addExtension("json");
}

void
TopoJSONLoader::handleCollection(const string & parent_id, Graph & graph, map<string, int> & nodes, map<int, vector<int> > & connections, Json::Value & objects, const vector<ArcData2D> & arc_data) {
  // ColumnArc & ag = dynamic_cast<ColumnArc &>(graph.getEdgeData()["_geometry"]);

  for (Json::ValueIterator it = objects.begin() ; it != objects.end() ; it++ ) {
    Json::Value object = *it;
    assert(object.type() == Json::objectValue);
    string type = object["type"].asString();
    string id_text = object["id"].asString();

    id_text = parent_id + "/" + id_text;

    cerr << "got type " << type << ", id " << id_text << endl;

    if (type == "Point") {
      Json::Value coordinates = object["coordinates"];
      glm::vec3 v(coordinates[0].asDouble(), coordinates[1].asDouble(), 0);
      ostringstream key;
      key << v.x << "/" << v.y;
      map<string, int>::iterator it = nodes.find(key.str());
      if (it != nodes.end()) {
	graph.getNodeData()["id"].setValue(it->second, id_text);
      } else {
	int node_id = nodes[key.str()] = graph.addNode();
	graph.getNodeData()["id"].setValue(it->second, id_text);
	graph.setPosition(node_id, v);
      }
    } else if (type == "MultiPoint") {
      assert(0);
    } else if (type == "GeometryCollection") {
      Json::Value geometries = object["geometries"];
      handleCollection(id_text, graph, nodes, connections, geometries, arc_data);
    } else {
      Json::Value arcs = object["arcs"];
      assert(arcs.type() == Json::arrayValue);
      if (type == "LineString" || type == "MultiLineString") {
	cerr << "skipping linestring\n";
#if 0
	int n = 0;
	for (unsigned int i = 0; i < arcs.size(); i++) {
	  int arc_id = arcs[i].asInt();
	  bool rev = false; 
	  if (arc_id < 0) {
	    arc_id = -arc_id - 1;
	    rev = true;
	  }
	  const Arc & arc = arc_data[arc_id];
	  pair<int, int> arc_nodes = createNodesForArc(arc, graph, nodes, rev);
	  
	  vector<int> & v = connections[arc_id];
	  assert(v.size() <= 1);
	  int target_id = v.size() ? v[0] : -1;
	  assert(target_id != node_id);
	  v.push_back(node_id);
	  if (target_id == -1) continue;
	  int arc_id = graph.addArcGeometry(arc); // should only be created once
	  int edge_id = rev ? graph.addEdge(target_id, node_id, -1, 1.0f, -arc_id) : graph.addEdge(node_id, target_id, -1, 1.0f, arc_id);
	}
#endif
      } else if (type == "Polygon")  {
	int region_id = graph.addRegion();
	if (!id_text.empty()) {
	  graph.getRegionData()["id"].setValue(region_id, id_text);
	}
	for (unsigned int i = 0; i < arcs.size(); i++) {
	  int face_id = graph.addFace(region_id);
	  Json::Value ring = arcs[i];
	  assert(ring.type() == Json::arrayValue);
	  for (unsigned int j = 0; j < ring.size(); j++) {
	    int arc_id = ring[j].asInt();
	    bool rev = false;	    
	    if (arc_id < 0) {
	      arc_id = -arc_id - 1;
	      rev = true;
	    }
	    const ArcData2D & arc = arc_data[arc_id];
	    pair<int, int> arc_nodes = createNodesForArc(arc, graph, nodes, rev);
	    vector<int> & v = connections[arc_id];
	    assert(v.size() <= 1);
	    int target_id = v.size() ? v[0] : -1;
	    assert(target_id != face_id);
	    v.push_back(face_id);
	    if (target_id == -1) continue;
	    int arc_id2 = graph.addArcGeometry(arc); // should only be created once
	    int edge_id = !rev ? graph.addEdge(arc_nodes.first, arc_nodes.second, face_id, 1.0f, arc_id2) : graph.addEdge(arc_nodes.first, arc_nodes.second, face_id, 1.0f, -arc_id2);
	    // add target_id
	    // ag.setArc(edge_id, arc);
	  }
	}
      } else if (type == "MultiPolygon") {
	int region_id = graph.addRegion();
	if (!id_text.empty()) {
	  graph.getRegionData()["id"].setValue(region_id, id_text);
	}
 	for (unsigned int h = 0; h < arcs.size(); h++) {
	  Json::Value polygon = arcs[h];
	  int face_id = graph.addFace(region_id);
	  assert(polygon.type() == Json::arrayValue);
	  for (unsigned int i = 0; i < polygon.size(); i++) {
	    Json::Value ring = polygon[i];
	    assert(ring.type() == Json::arrayValue);
	    for (unsigned int j = 0; j < ring.size(); j++) {
	      int arc_id = ring[j].asInt();
	      bool rev = false;
	      if (arc_id < 0) {
		arc_id = -arc_id - 1;
		rev = true;
	      }
	      const ArcData2D & arc = arc_data[arc_id];
	      pair<int, int> arc_nodes = createNodesForArc(arc, graph, nodes, rev);
	      vector<int> & v = connections[arc_id];
	      if (v.size() > 1) {
		cerr << "multiple connections(" << v.size() << ")\n";
	      }
	      int target_id = v.size() ? v[0] : -1;
	      if (target_id == face_id) {
		target_id = -1;
	      } else {
		v.push_back(face_id);
	      }
	      if (target_id == -1) continue;
	      int arc_id2 = graph.addArcGeometry(arc); // create only once
	      int edge_id = rev ? graph.addEdge(arc_nodes.first, arc_nodes.second, face_id, 1.0f, -arc_id2) : graph.addEdge(arc_nodes.first, arc_nodes.second, face_id, 1.0f, arc_id2);
	      // target_id, 
	      // ag.setArc(edge_id, arc);
	    }	    
	  }
	}	
      } else {
	cerr << "unhandled type " << type << "\n";
	assert(0);
      }
    }
  }
}

std::shared_ptr<Graph>
TopoJSONLoader::openGraph(const char * filename) {
  FILE * in = fopen(filename, "rt");
  string json;
  char buffer[4096];
  while (!feof(in)) {
    size_t r = fread(buffer, 1, 4096, in);
    if (!r) {
      break;
    } else {
      json += string(buffer, r);
    }
  }
  
  Json::Reader reader;
  Json::Value root;
  if (!reader.parse( json, root )) {
    cerr << "TopoJSONLoader.cpp: malformed json: " << json << endl;
    assert(0);
  }
  
  string root_type = root["type"].asString();
  Json::Value transform = root["transform"];
  Json::Value objects = root["objects"];
  Json::Value arcs = root["arcs"];

  auto graph = std::make_shared<PlanarGraph>();
  graph->setHasSpatialData(true);
  graph->setHasArcData(true);
  graph->getNodeData().addTextColumn("id");
  
  assert(root_type == "Topology");

  double scale_x = 1, scale_y = 1, translate_x = 0, translate_y = 0;
  if (transform.type() == Json::objectValue) {
    scale_x = transform["scale"][0].asDouble();
    scale_y = transform["scale"][1].asDouble();
    translate_x = transform["translate"][0].asDouble();
    translate_y = transform["translate"][1].asDouble();
  }

  // graph->getEdgeData().addArcColumn("_geometry");
  graph->getFaceData().addTextColumn("id");  
  graph->getRegionData().addTextColumn("id");
  
  map<string, int> nodes;
  map<int, vector<int> > connections;

  vector<ArcData2D> arc_data;
  for (unsigned int i = 0; i < arcs.size(); i++) {
    Json::Value d = arcs[i];
    assert(d.type() == Json::arrayValue);
    ArcData2D arc;
    double x = 0, y = 0;
    for (unsigned int j = 0; j < d.size(); j++) {
      Json::Value p = d[j];
      assert(p.type() == Json::arrayValue);
      x += p[0].asDouble();
      y += p[1].asDouble();
      arc.data.push_back(glm::dvec2(x * scale_x + translate_x,
				    y * scale_y + translate_y
				    ));
    }
    arc_data.push_back(arc);
  }

  handleCollection("", *graph, nodes, connections, objects, arc_data);

  return graph;
}
