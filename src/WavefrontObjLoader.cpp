#include "WavefrontObjLoader.h"

#include <cassert>
#include <sstream>
#include <iostream>
#include <fstream>

#include "PlanarGraph.h"
#include <StringUtils.h>

#include <glm/glm.hpp>

using namespace std;

WavefrontObjLoader::WavefrontObjLoader() : FileTypeHandler("WaveFront object", false) {
  addExtension("obj");
}

std::shared_ptr<Graph>
WavefrontObjLoader::openGraph(const char * filename, const std::shared_ptr<NodeArray> & initial_nodes) {
  auto graph = std::make_shared<PlanarGraph>();
  graph->setNodeArray(initial_nodes);
  graph->getNodeArray().setHasSpatialData(true);
  
  ifstream in(filename, ios::in);
  if (!in) {
    cerr << "Cannot open " << filename << endl;
    return 0;
  }
 
  map<string, int> nodes;
  map<string, int> waiting_faces;

  vector<glm::vec3> vertices;
  vector<glm::vec2> uvs;
  vector<glm::vec3> normals;

  // int region_id = graph->addRegion();
  
  string line;
  while (getline(in, line)) {
    if (line.substr(0,2) == "v ") {
      istringstream s(line.substr(2));
      double x, y, z;
      s >> x >> y >> z;
      vertices.push_back(glm::vec3(x, y, z));
    } else if (line.substr(0, 3) == "vn ") {
      istringstream s(line.substr(3));
      double x, y, z;
      s >> x;
      s >> y;
      s >> z;
      normals.push_back(glm::vec3(x, y, z));
    } else if (line.substr(0, 3) == "vt ") {
      istringstream s(line.substr(3));
      double x, y;
      s >> x >> y;
      uvs.push_back(glm::vec2(x, y));     
    } else if (line.substr(0, 2) == "f ") {      
      int face_id = graph->addFace();
      vector<unsigned int> face_nodes;
      istringstream s(line.substr(2));
      for (unsigned int i = 0; i < 3; i++) {
	string vs;
	s >> vs;
	vector<string> vd = StringUtils::split(vs, '/');
	assert(vd.size() == 3);
	int vi = stoi(vd[0]) - 1, uvi = stoi(vd[1]) - 1, ni = stoi(vd[2]) - 1;
	assert(vi >= 0 && vi < vertices.size());
	const glm::vec3 & v = vertices[vi];
#if 0
	assert(uvi >= 0 && uvi < uvs.size());
	const glm::vec2 & uv = uvs[uvi];
	assert(ni >= 0 && ni < normals.size());
	const glm::vec3 & normal = normals[ni];
#endif
	face_nodes.push_back(createNode3D(*graph, nodes, v.x, v.y, v.z));
      }
      assert(face_nodes.size() >= 3);
      for (int i = 0; i < (int)face_nodes.size(); i++) {
	int node1 = face_nodes[i], node2 = face_nodes[(i + 1) % face_nodes.size()];
	assert(node1 != node2);
	ostringstream key1, key2;
	key1 << node1 << "/" << node2;
	map<string, int>::iterator it = waiting_faces.find(key1.str());
	if (it != waiting_faces.end()) {
	  graph->addEdge(node1, node2, face_id, it->second);
	} else {
	  key2 << node2 << "/" << node1;
	  it = waiting_faces.find(key2.str());
	  if (it != waiting_faces.end()) {
	    graph->addEdge(node2, node1, it->second, face_id);
	  } else {
	    waiting_faces[key1.str()] = face_id;
	  }
	}
      }
    } else if (line[0] == '#') {
      // comment
    } else {
      cerr << "skipping row " << line << endl;
    }
  }

#if 0 
  normals.resize(mesh->vertices.size(), glm::vec3(0.0, 0.0, 0.0));
  for (int i = 0; i < elements.size(); i+=3) {
    GLushort ia = elements[i];
    GLushort ib = elements[i+1];
    GLushort ic = elements[i+2];
    glm::vec3 normal = glm::normalize(glm::cross( glm::vec3(vertices[ib]) - glm::vec3(vertices[ia]),
						  glm::vec3(vertices[ic]) - glm::vec3(vertices[ia])));
    normals[ia] = normals[ib] = normals[ic] = normal;
  }
#endif

  return graph;
}
