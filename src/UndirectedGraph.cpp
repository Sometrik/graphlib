#include "UndirectedGraph.h"

#include <GraphFilter.h>

using namespace std;
  
std::shared_ptr<Graph>
UndirectedGraph::createSimilar() const {
  auto graph = std::make_shared<UndirectedGraph>(getId());
  graph->setNodeVisibility(getNodeVisibility());
  graph->setEdgeVisibility(getEdgeVisibility());
  graph->setFaceVisibility(getFaceVisibility());
  graph->setLabelVisibility(getLabelVisibility());
  graph->setLineWidth(getLineWidth());
  graph->setNodeArray(nodes);
  
  return graph;
}
