#ifndef _BASICSIMPLIFIER_H_
#define _BASICSIMPLIFIER_H_

#include "GraphFilter.h"

#include <unordered_set>
#include <unordered_map>

struct node_tertiary_data_s;

class BasicSimplifier : public GraphFilter {
 public:
  BasicSimplifier() { }

  std::shared_ptr<GraphFilter> dup() const override { return std::make_shared<BasicSimplifier>(); }

  bool apply(Graph & target_graph, time_t start_time, time_t end_time, float start_sentiment, float end_sentiment, Graph & source_graph, RawStatistics & stats) override;
  void reset() override {
    seen_nodes.clear();
    seen_edges.clear();
    current_pos = -1;
    num_links = 0;
    num_hashtags = 0;
    min_time = 0;
    max_time = 0;
    // max_edge_weight = 0.0f;
    // max_node_coverage_weight = 0.0f;
  }
  bool hasPosition() const override { return current_pos != -1; }
  
 private:
  std::unordered_set<int> seen_nodes;
  std::unordered_map<int, std::unordered_map<int, int> > seen_edges;
  int current_pos = -1;
  unsigned int num_links = 0, num_hashtags = 0;
  time_t min_time = 0, max_time = 0;
};

class BasicSimplifierFactory : public GraphFilterFactory {
 public:
  BasicSimplifierFactory() { }

  virtual std::shared_ptr<GraphFilter> create() { return std::make_shared<BasicSimplifier>(); }
};

#endif
