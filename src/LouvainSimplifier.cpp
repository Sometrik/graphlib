#include "LouvainSimplifier.h"

#include <Graph.h>
#include <Louvain.h>
#include <iostream>

using namespace std;

LouvainSimplifier::LouvainSimplifier() {
  keepHashtags(true);
  keepLinks(true);
  keepLang(true);
  keepApplications(true);
}

bool
LouvainSimplifier::apply(Graph & target_graph, time_t start_time, time_t end_time, float start_sentiment, float end_sentiment, Graph & source_graph, RawStatistics & stats) {
  bool is_changed = processTemporalData(target_graph, start_time, end_time, start_sentiment, end_sentiment, source_graph, stats);
  
  if (is_changed) {
    target_graph.removeAllChildren();
    
    double precision = 0.000001;
    
    Louvain c(&target_graph, -1, precision);
    
    int level = 0;
    bool is_improved = true;
    
    for (int level = 1; is_improved; level++) {
      is_improved = c.oneLevel();

      if (is_improved) {
	for (auto cluster_id : c.getNodeIds()) {
	  auto & td = target_graph.getNodeTertiaryData(cluster_id);
	  assert(td.parent_node == -1);
	  float best_d = 0;
	  int best_node = -1;
	  for (int n = td.first_child; n != -1; ) {
	    auto & pd = target_graph.getNodeArray().getNodeData(n);
	    auto & ctd = target_graph.getNodeTertiaryData(n);
	    if (1) { // pd.type != NODE_ATTRIBUTE) {
	      if (ctd.group_leader != -1) {
		auto & ctd2 = target_graph.getNodeTertiaryData(ctd.group_leader);
		if (best_node == -1 || ctd2.weighted_indegree > best_d) {
		  best_node = ctd.group_leader;
		  best_d = ctd2.weighted_indegree;
		}
	      } else if (best_node == -1 || ctd.weighted_indegree > best_d) {
		best_node = n;
		best_d = ctd.weighted_indegree;
	      }
	    }
	    n = ctd.next_child;
	  }
	  target_graph.setGroupLeader(cluster_id, best_node);
	  target_graph.flattenChildren(cluster_id);
	}
      }

      // break;
    }
  }

  return is_changed;
}
