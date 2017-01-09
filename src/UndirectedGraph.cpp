#include "UndirectedGraph.h"

#include <GraphFilter.h>

using namespace std;
  
std::shared_ptr<Graph>
UndirectedGraph::createSimilar() const {
  std::shared_ptr<Graph> graph(new UndirectedGraph(getId()));
  graph->setNodeVisibility(getNodeVisibility());
  graph->setEdgeVisibility(getEdgeVisibility());
  graph->setFaceVisibility(getFaceVisibility());
  graph->setLabelVisibility(getLabelVisibility());
  graph->setLineWidth(getLineWidth());
  graph->setNodeArray(nodes);
  
  return graph;
}
