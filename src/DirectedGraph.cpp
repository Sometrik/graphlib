#include "DirectedGraph.h"

#include <cassert>
#include <iostream>

#include "RawStatistics.h"

using namespace std;

std::shared_ptr<Graph>
DirectedGraph::createSimilar() const {
  std::shared_ptr<Graph> graph(new DirectedGraph(getId()));
  graph->setNodeVisibility(getNodeVisibility());
  graph->setEdgeVisibility(getEdgeVisibility());
  graph->setFaceVisibility(getFaceVisibility());
  graph->setLabelVisibility(getLabelVisibility());
  graph->setLineWidth(getLineWidth());
  graph->setNodeArray(nodes);
  
  return graph;
}
