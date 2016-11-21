#ifndef _GROUPSIMPLIFIER_H_
#define _GROUPSIMPLIFIER_H_

#include "GraphFilter.h"

#include <unordered_set>
#include <unordered_map>

struct node_tertiary_data_s;

class GroupSimplifier : public GraphFilter {
 public:
  GroupSimplifier() { }

  bool updateData(Graph & target_graph, time_t start_time, time_t end_time, float start_sentiment, float end_sentiment, Graph & source_graph, RawStatistics & stats, bool is_first_level, Graph * base_graph);
  void reset() override {
    GraphFilter::reset();
    
    seen_nodes.clear();
    seen_edges.clear();
    onedegree_nodes.clear();
    zerodegree_nodes.clear();
    node_pairs.clear();
    node_triplets.clear();
    current_pos = -1;
    num_links = 0;
    num_hashtags = 0;
    min_time = 0;
    max_time = 0;
    // max_edge_weight = 0.0f;
    // max_node_coverage_weight = 0.0f;
  }
  bool hasPosition() const { return current_pos != -1; }
  
 protected:
  void breakNodePair(Graph & target_graph, int node_id);
  void breakOneDegreeNode(Graph & target_graph, int node_id);
  void breakZeroDegreeNode(Graph & target_graph, int node_id);
  bool canPair(int n1, int n2, const node_tertiary_data_s & td1, const node_tertiary_data_s & td2) const;

 private:
  std::unordered_set<int> seen_nodes;
  std::unordered_map<int, std::unordered_map<int, int> > seen_edges;
  std::unordered_map<int, int> onedegree_nodes;
  std::unordered_set<int> zerodegree_nodes;
  std::unordered_map<int, int> node_pairs;
  std::unordered_map<int, std::pair<int, int> > node_triplets;
  int current_pos = -1;
  unsigned int num_links = 0, num_hashtags = 0;
  time_t min_time = 0, max_time = 0;
};

#endif
