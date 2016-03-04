#include "FileTypeHandler.h"

#include <Graph.h>
#include <glm/glm.hpp>
#include <sstream>

using namespace std;

FileTypeHandler::FileTypeHandler(const string & _description, bool _can_write) : description(_description), can_write(_can_write) {

}

#if 0
glm::dvec2
FileTypeHandler:compute2DPolygonCentroid(const Point2D* vertices, int vertexCount) {
  glm::dvec2 centroid;
  double signedArea = 0.0;
  double x0 = 0.0; // Current vertex X
  double y0 = 0.0; // Current vertex Y
  double x1 = 0.0; // Next vertex X
  double y1 = 0.0; // Next vertex Y
  double a = 0.0;  // Partial signed area
  
  // For all vertices except last
  int i = 0;
  for (i = 0; i < vertexCount - 1; ++i) {
    x0 = vertices[i].x;
    y0 = vertices[i].y;
    x1 = vertices[i+1].x;
    y1 = vertices[i+1].y;
    a = x0*y1 - x1*y0;
    signedArea += a;
    centroid.x += (x0 + x1)*a;
    centroid.y += (y0 + y1)*a;
  }
  
  // Do last vertex
  x0 = vertices[i].x;
  y0 = vertices[i].y;
  x1 = vertices[0].x;
  y1 = vertices[0].y;
  a = x0*y1 - x1*y0;
  signedArea += a;
  centroid.x += (x0 + x1)*a;
  centroid.y += (y0 + y1)*a;
  
  signedArea *= 0.5;
  centroid.x /= (6.0*signedArea);
  centroid.y /= (6.0*signedArea);
  
  return centroid;
}
#endif

int
FileTypeHandler::createNode2D(Graph & graph, map<string, int> & nodes, double x, double y) {
  ostringstream key;
  key << x << "/" << y;
  map<string, int>::iterator it = nodes.find(key.str());
  if (it != nodes.end()) {
    return it->second;
  } else {
    int node_id = nodes[key.str()] = graph.addNode();
    graph.getNodeArray().setPosition(node_id, glm::vec3(x, y, 0.0f));
    return node_id;  
  }
}

int
FileTypeHandler::createNode3D(Graph & graph, map<string, int> & nodes, double x, double y, double z) {
  ostringstream key;
  key << x << "/" << y << "/" << z;
  map<string, int>::iterator it = nodes.find(key.str());
  if (it != nodes.end()) {
    return it->second;
  } else {
    int node_id = nodes[key.str()] = graph.addNode();
    graph.getNodeArray().setPosition(node_id, glm::vec3(x, y, z));
    return node_id;  
  }
}

pair<int, int>
FileTypeHandler::createNodesForArc(const ArcData2D & arc, Graph & graph, map<string, int> & nodes, bool rev) {
  assert(arc.data.size() >= 2);
  auto & v1 = arc.data.front();
  auto & v2 = arc.data.back();
  ostringstream key1, key2;
  key1 << v1.x << "/" << v1.y;
  key2 << v2.x << "/" << v2.y;
  int node1, node2;
  map<string, int>::iterator it = nodes.find(key1.str());
  if (it != nodes.end()) {
    node1 = it->second;
  } else {
    nodes[key1.str()] = node1 = graph.addNode();
    graph.getNodeArray().setPosition(node1, glm::vec3(v1.x, v1.y, 0.0f));
  }
  it = nodes.find(key2.str());
  if (it != nodes.end()) {
    node2 = it->second;
  } else {
    nodes[key2.str()] = node2 = graph.addNode();
    graph.getNodeArray().setPosition(node2, glm::vec3(v2.x, v2.y, 0.0f));
  }
  assert(node1 >= 0 && node2 >= 0);
  if (rev) {
    return pair<int, int>(node1, node2);
  } else {
    return pair<int, int>(node2, node1);
  }
}

std::shared_ptr<Graph>
FileTypeHandler::openGraph(const std::string & filename) {
  return openGraph(filename, std::make_shared<NodeArray>());
}
