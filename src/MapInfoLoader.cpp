#include "MapInfoLoader.h"

#include <cassert>
#include <sstream>
#include <iostream>

#include "PlanarGraph.h"
#include <StringUtils.h>

using namespace std;

MapInfoLoader::MapInfoLoader() : FileTypeHandler("MapInfo data interchange format", false) {
  addExtension("mif");

  commands["version"] = MIF_VERSION;
  commands["delimiter"] = MIF_DELIMITER;
  commands["columns"] = MIF_COLUMNS;
  commands["coordsys"] = MIF_COORDSYS;
  commands["charset"] = MIF_CHARSET;
  commands["data"] = MIF_DATA;

  primitives["point"] = MIF_POINT;
  primitives["line"] = MIF_LINE;
  primitives["pline"] = MIF_PLINE;
  primitives["region"] = MIF_REGION;
  primitives["pen"] = MIF_PEN;
  primitives["brush"] = MIF_BRUSH;

#if 0
  data_started = false;
  version = 0;
  delimiter = '\t';
  num_objects = 0;
  has_eof = false;
#endif
}

std::shared_ptr<Graph>
MapInfoLoader::openGraph(const char * filename, const std::shared_ptr<NodeArray> & initial_nodes) {
  auto graph = std::make_shared<PlanarGraph>();
  graph->setNodeArray(initial_nodes);
  graph->getNodeArray().setHasSpatialData(true);
  graph->getNodeArray().setHasArcData(true);

  ifstream in(filename, ios::in);
  if (!in) {
    cerr << "Cannot open " << filename << endl;
    return 0;
  }
  
  bool data_started = false;

  map<string, int> nodes;
  
  while (!in.eof() && !in.fail()) {
    string s;
    in >> s;
    StringUtils::trim(s);
    s = StringUtils::toLower(s);

    if (!s.size()) {
      continue;
    }
    
    cerr << "command = " << s << endl;
    
    if (!data_started) {
      map<string, MIFCommand>::iterator cmd = commands.find(s);
      if (cmd == commands.end()) {
	cerr << "command not found: " << s << endl;
	return 0;
      }
      if (cmd->second == MIF_DATA) {
	data_started = true;
      } else if (!handleCommand(cmd->second, in, *graph, nodes)) {
	cerr << "failed to handle command " << cmd->first;
	return 0;
      }
    } else {
      map<string, MIFPrimitive>::iterator cmd = primitives.find(s);
      if (cmd == primitives.end()) {
	cerr << "primitive not found: " << s << endl;
	return 0;
      }
      if (!handlePrimitive(cmd->second, in, *graph, nodes)) {
	cerr << "failed to handle command " << cmd->first;
	return 0;
      }
    }
  }
  
  cerr << "preparing filename\n";

  try {
    string mid = filename;
    string::size_type pos = mid.find_last_of(".");
    if (pos != string::npos) {
      mid.erase(pos);
      mid += ".mid";
      cerr << "loading csv " << mid << endl;
      graph->getFaceData().loadCSV(mid.c_str(), ',');
    }
    cerr << "done\n";
  } catch (exception & e) {
    cerr << "got exception while reading CSV: " << e.what() << endl;
    throw e;
  }
  
  return graph;
}

bool
MapInfoLoader::handleVersion(ifstream & in) {
  int version;
  in >> version;
  cerr << "version = " << version << endl;
  return true;
}

bool
MapInfoLoader::handleDelimiter(ifstream & in) {
  string s;
  in >> s;
  
  cerr << "delimiter: " << s << endl;

  return true;
}

bool
MapInfoLoader::handleCoordSys(ifstream & in, Graph & graph) {
  string s;
  getline(in, s);
  cerr << "coordsys: " << s << endl;

  return true;
}

bool
MapInfoLoader::handleCharset(ifstream & in) {
  string s;
  in >> s;

  return true;
}

bool
MapInfoLoader::handleColumns(ifstream & in, Graph & graph) {
  int num_columns = 0;
  in >> num_columns;

  cerr << "num columns: " << num_columns << endl;

  for (int i = 0; i < num_columns; i++) {
    string name, type;
    in >> name;
    in >> type;
    
    type = StringUtils::toLower(type);
    cerr << "column name: " << name << ", type: " << type << endl;

    if (type.compare(0, 4, "char") == 0 || 1) {
      graph.getFaceData().addTextColumn(name.c_str());
    } else if (type == "integer") {
      assert(0);
      graph.getFaceData().addIntColumn(name.c_str());
    } else {
      assert(0);
    }
  }

  return true;
}

bool
MapInfoLoader::handlePen(ifstream & in) {
  string pen;
  in >> pen;
  return true;
}

bool
MapInfoLoader::handleBrush(ifstream & in) {
  string pen;
  in >> pen;
  return true;
}

bool
MapInfoLoader::handlePoint(ifstream & in, Graph & graph, map<string, int> & nodes) {
  double x, y;
  in >> x;
  in >> y;

  ArcData2D arc;
  arc.data.push_back(glm::dvec2(x, y));
  
  int face_id = graph.addFace();
  int arc_id = graph.getNodeArray().addArcGeometry(arc);
  pair<int, int> n = createNodesForArc(arc, graph, nodes);
  assert(n.first == n.second);
  graph.addEdge(n.first, n.second, face_id, 1.0f, arc_id);
  
  return true;
}

bool
MapInfoLoader::handleLine(ifstream & in, Graph & graph, map<string, int> & nodes) {
  double x1, y1, x2, y2;
  in >> x1;
  in >> y1;
  in >> x2;
  in >> y2;

  ArcData2D arc;
  arc.data.push_back(glm::dvec2(x1, y1));
  arc.data.push_back(glm::dvec2(x2, y2));

  int face_id = graph.addFace();
  int arc_id = graph.getNodeArray().addArcGeometry(arc);
  pair<int, int> n = createNodesForArc(arc, graph, nodes);
  graph.addEdge(n.first, n.second, face_id, 1.0f, arc_id);
    
  return true;
}

bool
MapInfoLoader::handlePolyline(ifstream & in, Graph & graph, map<string, int> & nodes) {
  unsigned int num_points = 0;
  in >> num_points;  
  ArcData2D arc;
  for (unsigned int i = 0; i < num_points; i++) {
    double x, y;
    in >> x;
    in >> y;
    arc.data.push_back(glm::dvec2(x, y));
  }
  assert(arc.size() >= 2);
  int face_id = graph.addFace();
  int arc_id = graph.getNodeArray().addArcGeometry(arc);
  pair<int, int> n = createNodesForArc(arc, graph, nodes);
  graph.addEdge(n.first, n.second, face_id, 1.0f, arc_id);
  
  return true;
}

bool
MapInfoLoader::handleRegion(ifstream & in, Graph & graph, std::map<std::string, int> & nodes) {
  assert(0);
#if 0
  int num_contours = 0;
  in >> num_contours;
  for (int i = 0; i < num_contours; i++) {
    int num_points = 0;
    in >> num_points;
    for (int j = 0; j < num_points; j++) {
      double x, y;
      in >> x;
      in >> y;
      current_object.addVertex(glm::dvec2(x, y));
    }
    current_object.addPartOffset(current_object.getVertices().size());
  }
#endif
  return true;
}

bool
MapInfoLoader::handleCommand(MIFCommand cmd, ifstream & in, Graph & graph, map<string, int> & nodes) {
  switch (cmd) {
  case MIF_VERSION: return handleVersion(in);    
  case MIF_DELIMITER: return handleDelimiter(in);
  case MIF_COORDSYS: return handleCoordSys(in, graph);
  case MIF_COLUMNS: return handleColumns(in, graph);  
  case MIF_CHARSET: return handleCharset(in);
  case MIF_DATA: return false;
  }
  return false;
}

bool
MapInfoLoader::handlePrimitive(MIFPrimitive cmd, ifstream & in, Graph & graph, map<string, int> & nodes) {
  switch (cmd) {
  case MIF_POINT: return handlePoint(in, graph, nodes);
  case MIF_LINE: return handleLine(in, graph, nodes );
  case MIF_PLINE: return handlePolyline(in, graph, nodes);
  case MIF_REGION: return handleRegion(in, graph, nodes);
  case MIF_PEN: return handlePen(in);
  case MIF_BRUSH: return handleBrush(in);
  }
  return false;
}
