#ifndef _DIRECTEDGRAPH_H_
#define _DIRECTEDGRAPH_H_

#include <Graph.h>

#include <unordered_set>
#include <unordered_map>

class DirectedGraph : public Graph {
 public:
  DirectedGraph(int _id = 0);
  DirectedGraph(const DirectedGraph & other);

  std::shared_ptr<Graph> createSimilar() const override;
  Graph * copy() const override { return new DirectedGraph(*this); }
  bool isDirected() const override { return true; }
  bool hasPosition() const override { return current_pos != -1; }

  bool updateData(time_t start_time, time_t end_time, float start_sentiment, float end_sentiment, Graph & source_graph, RawStatistics & stats, bool is_first_level, Graph * base_graph) override;

  int addEdge(int n1, int n2, int face = -1, float weight = 1.0f, int arc = 0, long long coverage = 0) override;

  void clear() override {
    Graph::clear();
    edge_attributes.clear();
  }   
  
  edge_data_s & getEdgeAttributes(int i) override { return edge_attributes[i]; }
  const edge_data_s & getEdgeAttributes(int i) const override { return edge_attributes[i]; }
  EdgeIterator begin_edges() override { return EdgeIterator(&(edge_attributes.front()), sizeof(edge_data_s)); }
  EdgeIterator end_edges() override {
    EdgeIterator it(&(edge_attributes.back()), sizeof(edge_data_s));
    ++it;
    return it;
  }
  ConstEdgeIterator begin_edges() const override { return ConstEdgeIterator(&(edge_attributes.front()), sizeof(edge_data_s)); }
  ConstEdgeIterator end_edges() const override {
    ConstEdgeIterator it(&(edge_attributes.back()), sizeof(edge_data_s));
    ++it;
    return it;
  }

  void reset() override {
    seen_nodes.clear();
    seen_edges.clear();
    current_pos = -1;
    num_links = 0;
    num_hashtags = 0;
    min_time = 0;
    max_time = 0;
    max_edge_weight = 0.0f;
    max_node_coverage_weight = 0.0f;
  }
  
 private:
  std::vector<edge_data_s> edge_attributes;
  std::unordered_set<int> seen_nodes;
  std::unordered_map<int, std::unordered_map<int, int> > seen_edges;
  int current_pos = -1;
  unsigned int num_links = 0, num_hashtags = 0;
  time_t min_time = 0, max_time = 0;
};

#endif
