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
