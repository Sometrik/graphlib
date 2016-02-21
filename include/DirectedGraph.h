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
  
  void reset() override {
    seen_nodes.clear();
    seen_edges.clear();
    onedegree_nodes.clear();
    zerodegree_nodes.clear();
    current_pos = -1;
    num_links = 0;
    num_hashtags = 0;
    min_time = 0;
    max_time = 0;
    max_edge_weight = 0.0f;
    max_node_coverage_weight = 0.0f;
  }
  
 private:
  std::unordered_set<int> seen_nodes;
  std::unordered_map<int, std::unordered_map<int, int> > seen_edges;
  std::unordered_map<int, int> onedegree_nodes;
  std::unordered_set<int> zerodegree_nodes;
  int current_pos = -1;
  unsigned int num_links = 0, num_hashtags = 0;
  time_t min_time = 0, max_time = 0;
};

#endif
