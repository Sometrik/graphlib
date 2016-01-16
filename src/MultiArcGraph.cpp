#include "MultiArcGraph.h"

using namespace std;
using namespace table;

MultiArcGraph::MultiArcGraph(int _id) : Graph(1, _id) {
  setNodeSizeMethod(SizeMethod::SIZE_FROM_DEGREE);  
}

MultiArcGraph::MultiArcGraph(const MultiArcGraph & other)
  : Graph(other), edge_attributes(other.edge_attributes) {
  
}
  
std::shared_ptr<Graph>
MultiArcGraph::createSimilar() const {
  std::shared_ptr<Graph> graph(new MultiArcGraph(getId()));
  graph->setLocationGraphValid(false);
  graph->setAlpha3(getAlpha2());
  graph->setTemporal(isTemporal());
  graph->setPersonality(getPersonality());
  graph->setHasTextures(hasTextures());
  graph->setNodeSizeMethod(getNodeSizeMethod());
  graph->setClusterVisibility(getClusterVisibility());
  graph->setNodeVisibility(getNodeVisibility());
  graph->setEdgeVisibility(getEdgeVisibility());
  graph->setRegionVisibility(getRegionVisibility());
  graph->setLabelVisibility(getLabelVisibility());

  for (auto it = getNodeData().getColumns().begin(); it != getNodeData().getColumns().end(); it++) {
    if (it->first != "posts") {
      graph->getNodeData().addColumn(it->second->create());
    }
  }

  return graph;
}
