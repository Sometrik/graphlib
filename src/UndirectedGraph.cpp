#include "UndirectedGraph.h"

using namespace std;

UndirectedGraph::UndirectedGraph(int _id) : Graph(1, _id) {
}

UndirectedGraph::UndirectedGraph(const UndirectedGraph & other)
  : Graph(other), edge_attributes(other.edge_attributes) {
  
}
  
std::shared_ptr<Graph>
UndirectedGraph::createSimilar() const {
  std::shared_ptr<Graph> graph(new UndirectedGraph(getId()));
  graph->setLocationGraphValid(false);
  graph->setTemporal(isTemporal());
  graph->setPersonality(getPersonality());
  graph->setHasTextures(hasTextures());
  graph->setNodeArray(nodes);  

  return graph;
}

int
UndirectedGraph::addEdge(int n1, int n2, int face, float weight, int arc, long long coverage) {
    assert(n1 != -1 || n2 != -1);
    // assert(n1 != n2);
    int edge = (int)edge_attributes.size();

    int next_face_edge = -1;
    if (face != -1) next_face_edge = getFaceFirstEdge(face);

    int next_node_edge = -1;
    if (n1 != -1) {
      next_node_edge = getNodeFirstEdge(n1);
      setNodeFirstEdge(n1, edge);
      updateOutdegree(n1, 1);
      updateNodeSize(n1);
    }
    if (n2 != -1) {
      updateIndegree(n2, 1);
      updateNodeSize(n2);
    }

    edge_attributes.push_back(edge_data_s( weight, n1, n2, next_node_edge, face, next_face_edge, arc, coverage ));
    edges.addRow();
    total_edge_weight += fabsf(weight);

    incVersion();
    return edge;
  }
