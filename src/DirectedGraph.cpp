#include "DirectedGraph.h"

#include <cassert>
#include <iostream>

#include "RawStatistics.h"
#include "BasicSimplifier.h"

using namespace std;

DirectedGraph::DirectedGraph(int _id) : Graph(_id) {
  filter = std::make_shared<BasicSimplifier>();
}

DirectedGraph::DirectedGraph(const DirectedGraph & other) : Graph(other) {
  filter = std::make_shared<BasicSimplifier>();
}

std::shared_ptr<Graph>
DirectedGraph::createSimilar() const {
  std::shared_ptr<Graph> graph(new DirectedGraph(getId()));
  graph->setLocationGraphValid(false);
  graph->setNodeVisibility(getNodeVisibility());
  graph->setEdgeVisibility(getEdgeVisibility());
  graph->setFaceVisibility(getFaceVisibility());
  graph->setLabelVisibility(getLabelVisibility());
  graph->setLineWidth(getLineWidth());
  graph->setNodeArray(nodes);
  
  return graph;
}

bool
DirectedGraph::updateData(time_t start_time, time_t end_time, float start_sentiment, float end_sentiment, Graph & source_graph, RawStatistics & stats, bool is_first_level, Graph * base_graph) {
  if (filter.get()) {
    return filter->updateData(*this, start_time, end_time, start_sentiment, end_sentiment, source_graph, stats, is_first_level, base_graph);
  } else {
    return false;
  }
}
