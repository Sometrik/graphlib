#include "UndirectedGraph.h"

using namespace std;
using namespace table;

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
  graph->setClusterVisibility(getClusterVisibility());
  graph->setNodeVisibility(getNodeVisibility());
  graph->setEdgeVisibility(getEdgeVisibility());
  graph->setRegionVisibility(getRegionVisibility());
  graph->setLabelVisibility(getLabelVisibility());
  graph->getNodeArray().setNodeSizeMethod(getNodeArray().getNodeSizeMethod());
  graph->getNodeArray().setAlpha3(getNodeArray().getAlpha2());

  for (auto it = getNodeData().getColumns().begin(); it != getNodeData().getColumns().end(); it++) {
    if (it->first != "posts") {
      graph->getNodeData().addColumn(it->second->create());
    }
  }

  return graph;
}
