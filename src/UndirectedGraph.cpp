#include "UndirectedGraph.h"

using namespace std;

UndirectedGraph::UndirectedGraph(int _id) : Graph(1, _id) {
}

UndirectedGraph::UndirectedGraph(const UndirectedGraph & other)
  : Graph(other) {
  
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
