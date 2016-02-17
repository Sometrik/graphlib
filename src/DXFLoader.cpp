#include "DXFLoader.h"

#include "PlanarGraph.h"
#include "../../personal/system/StringUtils.h"

#include <string>
#include <sstream>
#include <map>

using namespace std;

#define GC_ENTITY_TYPE 		0
#define GC_BLOCK_NAME		2
#define GC_HANDLE		5
#define GC_LINETYPE_NAME	6
#define GC_LAYER		8
#define GC_LINETYPE_SCALE	48
#define GC_COLOR		62
#define GC_SUBCLASS_MARKER	100

class DXFBlock {
public:
  DXFBlock() { }
    DXFBlock(const string & _name, glm::dvec3 & _base_point) : name(_name), base_point(_base_point) { }

#if 0
  void addObject(GraphicsObject & o);
  std::vector<GraphicsObject> * getObjects() { return &objectlist; }
  void copyObjects(GraphicsLayer & layer, Matrix & matrix);
  void copyObjects(DXFBlock & block, Matrix & matrix); 
#endif
  void setDefaultColor(int c) { color = c; }
  
 private:
  std::string name;
  // std::vector<GraphicsObject> objectlist;
  int color;
  glm::dvec3 base_point;
};

class DXFEntity {
public:
  DXFEntity() { }
  virtual ~DXFEntity() { }
  
  int color = 7;
};

class DXFInsert : public DXFEntity {
public:
  DXFInsert(const std::string & _block_name) : block_name(_block_name) { }

private:
  string block_name;
};

class DXFLine : public DXFEntity {
 public:
  DXFLine() { }
  std::vector<glm::vec3> v;
};

class DXFPoint : public DXFEntity {
 public:
  DXFPoint() { }
  std::vector<glm::vec3> v;
};

class DXFFace : public DXFEntity {
 public:
  DXFFace() { }

  void calculateNormal() {
    if (v.size() >= 3) {
      glm::vec3 v01 = v[0] - v[1];
      glm::vec3 v02 = v[0] - v[2];
      normal = glm::normalize(glm::cross(v01, v02));
    }
  }
  
  std::vector<glm::vec3> v;
  glm::vec3 normal;
};

class DXFLayer {
 public:
  DXFLayer() { color = -1; }
  
  string name;
  int color;
};

struct DXFColor {
  DXFColor(int _red, int _green, int _blue)
    : red(_red), green(_green), blue(_blue) { }

  unsigned char red, green, blue;
};

static const DXFColor aci_colors[256] = {
  /*   0 */ {255, 255, 255},
  /*   1 */ {255,   0,   0},
  /*   2 */ {255, 255,   0},
  /*   3 */ {  0, 255,   0},
  /*   4 */ {  0, 255, 255},
  /*   5 */ {  0,   0, 255},
  /*   6 */ {255,   0, 255},
  /*   7 */ {255, 255, 255},
  /*   8 */ {128, 128, 128},
  /*   9 */ {192, 192, 192},
  /*  10 */ {255,   0,   0},
  /*  11 */ {255, 127, 127},
  /*  12 */ {204,   0,   0},
  /*  13 */ {204, 102, 102},
  /*  14 */ {153,   0,   0},
  /*  15 */ {153,  76,  76},
  /*  16 */ {127,   0,   0},
  /*  17 */ {127,  63,  63},
  /*  18 */ { 76,   0,   0},
  /*  19 */ { 76,  38,  38},
  /*  20 */ {255,  63,   0},
  /*  21 */ {255, 159, 127},
  /*  22 */ {204,  51,   0},
  /*  23 */ {204, 127, 102},
  /*  24 */ {153,  38,   0},
  /*  25 */ {153,  95,  76},
  /*  26 */ {127,  31,   0},
  /*  27 */ {127,  79,  63},
  /*  28 */ { 76,  19,   0},
  /*  29 */ { 76,  47,  38},
  /*  30 */ {255, 127,   0},
  /*  31 */ {255, 191, 127},
  /*  32 */ {204, 102,   0},
  /*  33 */ {204, 153, 102},
  /*  34 */ {153,  76,   0},
  /*  35 */ {153, 114,  76},
  /*  36 */ {127,  63,   0},
  /*  37 */ {127,  95,  63},
  /*  38 */ { 76,  38,   0},
  /*  39 */ { 76,  57,  38},
  /*  40 */ {255, 191,   0},
  /*  41 */ {255, 223, 127},
  /*  42 */ {204, 153,   0},
  /*  43 */ {204, 178, 102},
  /*  44 */ {153, 114,   0},
  /*  45 */ {153, 133,  76},
  /*  46 */ {127,  95,   0},
  /*  47 */ {127, 111,  63},
  /*  48 */ { 76,  57,   0},
  /*  49 */ { 76,  66,  38},
  /*  50 */ {255, 255,   0},
  /*  51 */ {255, 255, 127},
  /*  52 */ {204, 204,   0},
  /*  53 */ {204, 204, 102},
  /*  54 */ {153, 153,   0},
  /*  55 */ {153, 153,  76},
  /*  56 */ {127, 127,   0},
  /*  57 */ {127, 127,  63},
  /*  58 */ { 76,  76,   0},
  /*  59 */ { 76,  76,  38},
  /*  60 */ {191, 255,   0},
  /*  61 */ {223, 255, 127},
  /*  62 */ {153, 204,   0},
  /*  63 */ {178, 204, 102},
  /*  64 */ {114, 153,   0},
  /*  65 */ {133, 153,  76},
  /*  66 */ { 95, 127,   0},
  /*  67 */ {111, 127,  63},
  /*  68 */ { 57,  76,   0},
  /*  69 */ { 66,  76,  38},
  /*  70 */ {127, 255,   0},
  /*  71 */ {191, 255, 127},
  /*  72 */ {102, 204,   0},
  /*  73 */ {153, 204, 102},
  /*  74 */ { 76, 153,   0},
  /*  75 */ {114, 153,  76},
  /*  76 */ { 63, 127,   0},
  /*  77 */ { 95, 127,  63},
  /*  78 */ { 38,  76,   0},
  /*  79 */ { 57,  76,  38},
  /*  80 */ { 63, 255,   0},
  /*  81 */ {159, 255, 127},
  /*  82 */ { 51, 204,   0},
  /*  83 */ {127, 204, 102},
  /*  84 */ { 38, 153,   0},
  /*  85 */ { 95, 153,  76},
  /*  86 */ { 31, 127,   0},
  /*  87 */ { 79, 127,  63},
  /*  88 */ { 19,  76,   0},
  /*  89 */ { 47,  76,  38},
  /*  90 */ {  0, 255,   0},
  /*  91 */ {127, 255, 127},
  /*  92 */ {  0, 204,   0},
  /*  93 */ {102, 204, 102},
  /*  94 */ {  0, 153,   0},
  /*  95 */ { 76, 153,  76},
  /*  96 */ {  0, 127,   0},
  /*  97 */ { 63, 127,  63},
  /*  98 */ {  0,  76,   0},
  /*  99 */ { 38,  76,  38},
  /* 100 */ {  0, 255,  63},
  /* 101 */ {127, 255, 159},
  /* 102 */ {  0, 204,  51},
  /* 103 */ {102, 204, 127},
  /* 104 */ {  0, 153,  38},
  /* 105 */ { 76, 153,  95},
  /* 106 */ {  0, 127,  31},
  /* 107 */ { 63, 127,  79},
  /* 108 */ {  0,  76,  19},
  /* 109 */ { 38,  76,  47},
  /* 110 */ {  0, 255, 127},
  /* 111 */ {127, 255, 191},
  /* 112 */ {  0, 204, 102},
  /* 113 */ {102, 204, 153},
  /* 114 */ {  0, 153,  76},
  /* 115 */ { 76, 153, 114},
  /* 116 */ {  0, 127,  63},
  /* 117 */ { 63, 127,  95},
  /* 118 */ {  0,  76,  38},
  /* 119 */ { 38,  76,  57},
  /* 120 */ {  0, 255, 191},
  /* 121 */ {127, 255, 223},
  /* 122 */ {  0, 204, 153},
  /* 123 */ {102, 204, 178},
  /* 124 */ {  0, 153, 114},
  /* 125 */ { 76, 153, 133},
  /* 126 */ {  0, 127,  95},
  /* 127 */ { 63, 127, 111},
  /* 128 */ {  0,  76,  57},
  /* 129 */ { 38,  76,  66},
  /* 130 */ {  0, 255, 255},
  /* 131 */ {127, 255, 255},
  /* 132 */ {  0, 204, 204},
  /* 133 */ {102, 204, 204},
  /* 134 */ {  0, 153, 153},
  /* 135 */ { 76, 153, 153},
  /* 136 */ {  0, 127, 127},
  /* 137 */ { 63, 127, 127},
  /* 138 */ {  0,  76,  76},
  /* 139 */ { 38,  76,  76},
  /* 140 */ {  0, 191, 255},
  /* 141 */ {127, 223, 255},
  /* 142 */ {  0, 153, 204},
  /* 143 */ {102, 178, 204},
  /* 144 */ {  0, 114, 153},
  /* 145 */ { 76, 133, 153},
  /* 146 */ {  0,  95, 127},
  /* 147 */ { 63, 111, 127},
  /* 148 */ {  0,  57,  76},
  /* 149 */ { 38,  66,  76},
  /* 150 */ {  0, 127, 255},
  /* 151 */ {127, 191, 255},
  /* 152 */ {  0, 102, 204},
  /* 153 */ {102, 153, 204},
  /* 154 */ {  0,  76, 153},
  /* 155 */ { 76, 114, 153},
  /* 156 */ {  0,  63, 127},
  /* 157 */ { 63,  95, 127},
  /* 158 */ {  0,  38,  76},
  /* 159 */ { 38,  57,  76},
  /* 160 */ {  0,  63, 255},
  /* 161 */ {127, 159, 255},
  /* 162 */ {  0,  51, 204},
  /* 163 */ {102, 127, 204},
  /* 164 */ {  0,  38, 153},
  /* 165 */ { 76,  95, 153},
  /* 166 */ {  0,  31, 127},
  /* 167 */ { 63,  79, 127},
  /* 168 */ {  0,  19,  76},
  /* 169 */ { 38,  47,  76},
  /* 170 */ {  0,   0, 255},
  /* 171 */ {127, 127, 255},
  /* 172 */ {  0,   0, 204},
  /* 173 */ {102, 102, 204},
  /* 174 */ {  0,   0, 153},
  /* 175 */ { 76,  76, 153},
  /* 176 */ {  0,   0, 127},
  /* 177 */ { 63,  63, 127},
  /* 178 */ {  0,   0,  76},
  /* 179 */ { 38,  38,  76},
  /* 180 */ { 63,   0, 255},
  /* 181 */ {159, 127, 255},
  /* 182 */ { 51,   0, 204},
  /* 183 */ {127, 102, 204},
  /* 184 */ { 38,   0, 153},
  /* 185 */ { 95,  76, 153},
  /* 186 */ { 31,   0, 127},
  /* 187 */ { 79,  63, 127},
  /* 188 */ { 19,   0,  76},
  /* 189 */ { 47,  38,  76},
  /* 190 */ {127,   0, 255},
  /* 191 */ {191, 127, 255},
  /* 192 */ {102,   0, 204},
  /* 193 */ {153, 102, 204},
  /* 194 */ { 76,   0, 153},
  /* 195 */ {114,  76, 153},
  /* 196 */ { 63,   0, 127},
  /* 197 */ { 95,  63, 127},
  /* 198 */ { 38,   0,  76},
  /* 199 */ { 57,  38,  76},
  /* 200 */ {191,   0, 255},
  /* 201 */ {223, 127, 255},
  /* 202 */ {153,   0, 204},
  /* 203 */ {178, 102, 204},
  /* 204 */ {114,   0, 153},
  /* 205 */ {133,  76, 153},
  /* 206 */ { 95,   0, 127},
  /* 207 */ {111,  63, 127},
  /* 208 */ { 57,   0,  76},
  /* 209 */ { 66,  38,  76},
  /* 210 */ {255,   0, 255},
  /* 211 */ {255, 127, 255},
  /* 212 */ {204,   0, 204},
  /* 213 */ {204, 102, 204},
  /* 214 */ {153,   0, 153},
  /* 215 */ {153,  76, 153},
  /* 216 */ {127,   0, 127},
  /* 217 */ {127,  63, 127},
  /* 218 */ { 76,   0,  76},
  /* 219 */ { 76,  38,  76},
  /* 220 */ {255,   0, 191},
  /* 221 */ {255, 127, 223},
  /* 222 */ {204,   0, 153},
  /* 223 */ {204, 102, 178},
  /* 224 */ {153,   0, 114},
  /* 225 */ {153,  76, 133},
  /* 226 */ {127,   0,  95},
  /* 227 */ {127,  63, 111},
  /* 228 */ { 76,   0,  57},
  /* 229 */ { 76,  38,  66},
  /* 230 */ {255,   0, 127},
  /* 231 */ {255, 127, 191},
  /* 232 */ {204,   0, 102},
  /* 233 */ {204, 102, 153},
  /* 234 */ {153,   0,  76},
  /* 235 */ {153,  76, 114},
  /* 236 */ {127,   0,  63},
  /* 237 */ {127,  63,  95},
  /* 238 */ { 76,   0,  38},
  /* 239 */ { 76,  38,  57},
  /* 240 */ {255,   0,  63},
  /* 241 */ {255, 127, 159},
  /* 242 */ {204,   0,  51},
  /* 243 */ {204, 102, 127},
  /* 244 */ {153,   0,  38},
  /* 245 */ {153,  76,  95},
  /* 246 */ {127,   0,  31},
  /* 247 */ {127,  63,  79},
  /* 248 */ { 76,   0,  19},
  /* 249 */ { 76,  38,  47},
  /* 250 */ { 51,  51,  51},
  /* 251 */ { 91,  91,  91},
  /* 252 */ {132, 132, 132},
  /* 253 */ {173, 173, 173},
  /* 254 */ {214, 214, 214},
  /* 255 */ {255, 255, 255}
};

static DXFColor fromACIColor(int col) {
  if (col == 256) { // unknown color
    return DXFColor(255, 255, 255);
  }
  if (!(col >= 0 && col <= 255)) {
    cerr << "bad color " << col << endl;
    assert(0);
  }
  return aci_colors[col];
}

static void getLines(ifstream & stream, string & line1, string & line2) {
  getline(stream, line1);
  getline(stream, line2);
  StringUtils::trim(line1);
  StringUtils::trim(line2);
}

DXFLoader::DXFLoader() : FileTypeHandler("AutoCAD DXF", false) {
  addExtension("dxf");
}

const DXFLayer &
DXFLoader::getLayer(const string & layer, list<DXFLayer> & layers) const {
  static DXFLayer null_layer;
  for (auto & l : layers) {
    if (l.name == layer) {
      return l;
    }
  }
  return null_layer;
}

bool
DXFLoader::parseHeader(ifstream & stream) {
  cerr << "parsing header\n";
  string line1, line2;
  while (!stream.eof() && !stream.fail()) {
    getLines(stream, line1, line2);
    if (line1 == "0" && line2 == "ENDSEC") {
      return true;
    }
  }
  return false;
}

bool
DXFLoader::parseTables(ifstream & stream, list<DXFLayer> & layers) {
  string line1, line2;
  int state = 1;
  DXFLayer layer;
  while (!stream.eof() && !stream.fail()) {
    getLines(stream, line1, line2);
    if (line1 == "0" && state == 1) {
      if (!layer.name.empty() && layer.color != -1) {
	cerr << "Got layer: " << layer.name << endl;
	layers.push_back(layer);
      }
      layer = DXFLayer();
      state = 0;
    }
    
    if (line1 == "0" && line2 == "ENDSEC") {
      return true;
    } else if (line1 == "0" && line2 == "LAYER") {
      state = 1;
    } else if (state == 1) {
      if (line1 == "2") { // Layer name
	layer.name = line2;
      } else if (line1 == "62") { // Color
	layer.color = stoi(line2);
      }
    }
  }
  return false;
}

bool
DXFLoader::parseEntities(ifstream & stream, list<DXFLayer> & layers, Graph & graph, map<string, int> & nodes, map<string, int> & waiting_faces) {
  int activeEntity = 0;
  string line1, line2;
  int color = -1;
  string layer, block_name;
  std::vector<glm::vec3> v;
  std::vector<DXFInsert> inserts;
  
  while (!stream.eof() && !stream.fail()) {
    getLines(stream, line1, line2);
    int group_code = stoi(line1);
    if (group_code == GC_ENTITY_TYPE && activeEntity > 0) {
      // new entity starts, finish current
      if (activeEntity == 1) { // 3DFACE
	DXFFace p;
	p.v = v;
	p.calculateNormal();
	if (color != -1) {
	  p.color = color;
	} else {
	  p.color = getLayer(layer, layers).color;
	}
	process3DFace(graph, nodes, waiting_faces, p);
	
	color = -1;
	layer = "";
	v.clear();
	activeEntity = 0;
      } else if (activeEntity == 2) { // LINE
	DXFLine p;
	p.v = v;
	assert(p.v.size() == 2);
	if (color != -1) {
	  p.color = color;
	} else {
	  p.color = getLayer(layer, layers).color;
	}
	processLine(graph, nodes, p);
	color = -1;
	layer = "";
	v.clear();
	activeEntity = 0;
      } else if (activeEntity == 3) { // INSERT
	DXFInsert insert(block_name);
	
	activeEntity = 0;	
	v.clear();
      } else if (activeEntity == 4) { // POINT
	activeEntity = 0;
	v.clear();
      }
    }
    if (group_code == GC_ENTITY_TYPE) {
      if (line2 == "ENDSEC") {
	return true;
      } else if (line2 == "3DFACE") {
	activeEntity = 1;
      } else if (line2 == "LINE") {
	activeEntity = 2;
      } else if (line2 == "INSERT") {
	activeEntity = 3;
      } else if (line2 == "POINT") {
	activeEntity = 4;
      } else {
	cerr << "unhandled entity type " << line2 << endl;
      }
    } else if (activeEntity > 0) {
      switch (group_code) {
      case GC_BLOCK_NAME:
	block_name = line2;
	break;
      case GC_HANDLE:
	cerr << "got handle: " << line2 << endl;
	break;
      case GC_LAYER:
	cerr << "got entity layer: " << line2 << endl;
	layer = line2;
	break;
      case 10:
	if (v.size() < 1) v.resize(1);
	v[0].x = stod(line2);
	break;
      case 20:
	if (v.size() < 1) v.resize(1);
	v[0].y = stod(line2);
	break;
      case 30:
	if (v.size() < 1) v.resize(1);
	v[0].z = stod(line2);
	break;
      case 11:
	if (v.size() < 2) v.resize(2);
	v[1].x = stod(line2);
	break;
      case 21:
	if (v.size() < 2) v.resize(2);
	v[1].y = stod(line2);
	break;
      case 31:
	if (v.size() < 2) v.resize(2);
	v[1].z = stod(line2);
	break;
      case 12:
	if (v.size() < 3) v.resize(3);
	v[2].x = stod(line2);
	break;
      case 22:
	if (v.size() < 3) v.resize(3);
	v[2].y = stod(line2);
	break;
      case 32:
	if (v.size() < 3) v.resize(3);
	v[2].z = stod(line2);
	break;
      case 13:
	if (v.size() < 4) v.resize(4);
	v[3].x = stod(line2);
	break;
      case 23:
	if (v.size() < 4) v.resize(4);
	v[3].y = stod(line2);
	break;
      case 33:
	if (v.size() < 4) v.resize(4);
	v[3].z = stod(line2);
	break;
      case GC_COLOR: // color
	color = stoi(line2);
	break;
      default:
	cerr << "unhandled group code: " << group_code << " " << line2 << endl;
      }
    } else {
      cerr << "unused group entity: " << group_code << " " << line2 << endl;
    }
  }
  return false;
}

std::shared_ptr<Graph>
DXFLoader::openGraph(const char * filename) {
  ifstream stream(filename, ios::in);
  if (!stream) {
    cerr << "Cannot open " << filename << endl;
    return 0;
  }

  list<DXFLayer> layers;
  list<std::shared_ptr<DXFEntity> > entities;
  map<string, int> nodes;
  map<string, int> waiting_faces;

  auto graph = std::make_shared<PlanarGraph>();
  graph->setNodeArray(std::make_shared<NodeArray>());
  graph->getNodeArray().setFaceVisibility(true);
  graph->setHasSpatialData(true);

  string line1, line2;
  while (!stream.eof() && !stream.fail()) {
    getLines(stream, line1, line2);
    if (line1 == "999") {
      continue; // comment
    } else if (line1 == "0" && line2 == "SECTION") {
      getLines(stream, line1, line2);
      if (line1 == "2") {
	if (line2 == "HEADER") {
	  if (!parseHeader(stream)) {
	    return 0;
	  }
	} else if (line2 == "TABLES") {
	  if (!parseTables(stream, layers)) {
	    return 0;
	  }
	} else if (line2 == "ENTITIES") {
	  if (!parseEntities(stream, layers, *graph, nodes, waiting_faces)) {
	    return 0;
	  }
	}
      }
    }
  } 

  return graph;
}

void
DXFLoader::process3DFace(Graph & graph, map<string, int> & nodes, map<string, int> & waiting_faces, const DXFFace & face) {
  DXFColor c = fromACIColor(face.color);
  // int region_id = graph.addRegion();
  int face_id = graph.addFace();
  graph.setFaceColor(face_id, { c.red, c.green, c.blue });
  graph.setFaceNormal(face_id, face.normal);
  vector<int> face_nodes;      
  for (auto & v : face.v) {
    face_nodes.push_back(createNode3D(graph, nodes, v.x, v.y, v.z));	
  }
  assert(face_nodes.size() >= 3);
  for (int i = 0; i < face_nodes.size(); i++) {
    int node1 = face_nodes[i], node2 = face_nodes[(i + 1) % face_nodes.size()];
    if (node1 == node2) {
      // cerr << "skipping doubled vertex\n";
    } else {
      ostringstream key1, key2;
      key1 << node1 << "/" << node2;
      map<string, int>::iterator it = waiting_faces.find(key1.str());
      if (it != waiting_faces.end()) {
	graph.addEdge(node1, node2, face_id, it->second);
      } else {
	key2 << node2 << "/" << node1;
	it = waiting_faces.find(key2.str());
	if (it != waiting_faces.end()) {
	  graph.addEdge(node2, node1, it->second, face_id);
	} else {
	  waiting_faces[key1.str()] = face_id;
	}
      }
    }
  }      
}

void
DXFLoader::processLine(Graph & graph, map<string, int> & nodes, const DXFLine & line) {
  DXFColor c = fromACIColor(line.color);
  vector<int> line_nodes;
  for (auto & v : line.v) {
    int node_id = createNode3D(graph, nodes, v.x, v.y, v.z);
    line_nodes.push_back(node_id);
  }
  assert(line_nodes.size() == 2);
  assert(line_nodes[0] != line_nodes[1]);
  // BUG: edge might already exist
  graph.addEdge(line_nodes[0], line_nodes[1]);
}
