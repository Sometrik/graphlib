#include "UndirectedGraph.h"

#include <GraphFilter.h>

using namespace std;

UndirectedGraph::UndirectedGraph(int _id) : Graph(_id) {
}

UndirectedGraph::UndirectedGraph(const UndirectedGraph & other)
  : Graph(other) {
  
}
  
std::shared_ptr<Graph>
UndirectedGraph::createSimilar() const {
  std::shared_ptr<Graph> graph(new UndirectedGraph(getId()));
  graph->setLocationGraphValid(false);
  graph->setNodeVisibility(getNodeVisibility());
  graph->setEdgeVisibility(getEdgeVisibility());
  graph->setFaceVisibility(getFaceVisibility());
  graph->setLabelVisibility(getLabelVisibility());
  graph->setLineWidth(getLineWidth());
  graph->setNodeArray(nodes);
  if (getFilter().get()) graph->setFilter(getFilter()->dup());
  
  return graph;
}
