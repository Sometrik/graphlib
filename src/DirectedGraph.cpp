#include "DirectedGraph.h"

#include <cassert>
#include <iostream>

#include "RawStatistics.h"

using namespace std;
using namespace table;

DirectedGraph::DirectedGraph(int _id) : Graph(1, _id) {
  setNodeSizeMethod(SizeMethod(SizeMethod::SIZE_FROM_INDEGREE));
}

DirectedGraph::DirectedGraph(const DirectedGraph & other)
  : Graph(other), edge_attributes(other.edge_attributes) {
  
}

std::shared_ptr<Graph>
DirectedGraph::createSimilar() const {
  std::shared_ptr<Graph> graph(new DirectedGraph(getId()));
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
  graph->setLineWidth(getLineWidth());
  graph->setLabelStyle(getLabelStyle());
  graph->setDefaultSymbolId(getDefaultSymbolId());
  
  for (auto it = getNodeData().getColumns().begin(); it != getNodeData().getColumns().end(); it++) {
    graph->getNodeData().addColumn(it->second->create());
  }

  return graph;
}

#if 0
struct component_s {
  component_s() { }
  unsigned int size = 0;
};

Graph *
DirectedGraph::simplify() const {
  vector<component_s> components;
  map<int, int> nodes_to_component;
  
  for (int v = 0; v < getNodeCount(); v++) {
    int found_component = -1;
    int edge = getNodeFirstEdge(v);
    while (edge != -1) {
      int succ = getEdgeTargetNode(edge);
      auto it = nodes_to_component.find(succ);
      if (it != nodes_to_component.end()) {
	found_component = it->second;
	break;
      }	
      edge = getNextNodeEdge(edge);
    }
    if (found_component == -1) {
      found_component = components.size();
      components.push_back(component_s());
    }
    nodes_to_component[v] = found_component;
  }
    
  unsigned int singles_count = 0, pairs_count = 0;
  
  for (auto c : components) {
    if (c.size == 1) {
      singles_count++;
    } else if (c.size == 2) {
      pairs_count++;
    }
  }
  
  DirectedGraph * new_graph = new DirectedGraph();
  int singles_node = -1, pairs_node = -1;
  if (singles_count) {
    singles_node = new_graph->addNode();
  }
  if (pairs_count) {
    pairs_node = new_graph->addNode();
  }
  
  for (int v = 0; v < getNodeCount(); v++) {
    int component_id = nodes_to_component[v];
    auto & c = components[component_id];
    if (c.size == 1 || c.size == 2) {
      
    } else {
      
    }
  }

  return new_graph;
}
#endif
