#include "MultiArcGraph.h"

using namespace std;

MultiArcGraph::MultiArcGraph(int _id) : Graph(1, _id) {  
}

MultiArcGraph::MultiArcGraph(const MultiArcGraph & other)
  : Graph(other), edge_attributes(other.edge_attributes) {
  
}
  
std::shared_ptr<Graph>
MultiArcGraph::createSimilar() const {
  std::shared_ptr<Graph> graph(new MultiArcGraph(getId()));
  graph->setLocationGraphValid(false);
  graph->setTemporal(isTemporal());
  graph->setPersonality(getPersonality());
  graph->setHasTextures(hasTextures());
  graph->setNodeArray(nodes);
  
  return graph;
}

int
MultiArcGraph::addEdge(int n1, int n2, int face, float weight, int arc, long long coverage) {
  assert(n1 != -1 && n2 != -1);
  int edge = (int)edge_attributes.size();
  
  int next_node_edge = getNodeFirstEdge(n1);
  setNodeFirstEdge(n1, edge);
  if (n1 != n2) {
    updateOutdegree(n1, 1.0f); // weight
    updateIndegree(n2, 1.0f); // weight
  }
  updateNodeSize(n1);
  updateNodeSize(n2);
  
  if (face != -1) {
    
  }
  
  edge_attributes.push_back(edge_data_s( weight, n1, n2, next_node_edge, face, -1, arc ));
  edges.addRow();
  total_edge_weight += fabsf(weight);
  
  incVersion();
  return edge;
}
