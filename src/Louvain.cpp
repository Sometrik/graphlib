#include "Louvain.h"

#include <Graph.h>

#include <iostream>
#include <cassert>

using namespace std;

Louvain::Louvain(Graph * _g, int _max_num_passes, double _min_modularity,
		 bool _create_node_clusters, bool _create_edge_clusters)
  : g(_g),
    max_num_passes(_max_num_passes),
    min_modularity(_min_modularity),
    create_node_clusters(_create_node_clusters),
    create_edge_clusters(_create_edge_clusters)
{
  if (create_node_clusters) {
    auto end = g->end_visible_nodes();
    for (auto it = g->begin_visible_nodes(); it != end; ++it) {
      if (getNodeCommunity(*it) == -1) {
	nodes.push_back(*it);
	int community_id = g->getNodeArray().createCommunity(*it);
	g->addChild(community_id, *it, 0);
      }
    }
  }
#if 0
  if (create_edge_clusters) {
    auto end = g->end_edges();
    for (auto it = g->begin_edges(); it != end; ++it) {
      if (getEdgeCommunity(*it) == -1) {
	edges.push_back(*it);
	int community_id = g->getNodeArray().createCommunity(*it);
	g->addEdgeChild(community_id, *it, 0);
      }
    }
  }
#endif
}

int
Louvain::getNodeCommunity(int node) const {
  auto & td = getGraph().getNodeTertiaryData(node);
  return td.parent_node;
}

unordered_map<int, float>
Louvain::neighboringCommunities(int node) const {
  unordered_map<int, float> comms;

  comms[getNodeCommunity(node)] = 0.0f;
    
  auto nd = g->getAllNeighbors(node);
  for (auto n : nd) {
    unsigned int neigh = n.first;
    unsigned int neigh_comm = getNodeCommunity(neigh);
    float neigh_w = n.second;
    if (neigh != node) {
      auto it = comms.find(neigh_comm);
      if (it != comms.end()) {
	it->second += neigh_w;
      } else {
	comms[neigh_comm] = neigh_w;
      }      
    }
  }

  return comms;
}

bool
Louvain::oneLevel() {
  bool is_improved = false;
  double modularity = getGraph().modularity();

  // shuffle nodes
  for (int i = 0; i < nodes.size()-1; i++) {
    int rand_pos = rand() % (nodes.size()-i)+i;
    int tmp = nodes[i];
    nodes[i] = nodes[rand_pos];
    nodes[rand_pos] = tmp;
  }

  // repeat until there is no improvement in modularity, or the improvement is smaller than epsilon, or maximum number of passes have been done  
  for (int num_passes = 0; max_num_passes == -1 || num_passes < max_num_passes; num_passes++) {
    int num_moves = 0;
    num_passes++;

    // for each node: remove the node from its community and insert it in the best community
    for (int node : nodes) {
      int node_comm = getNodeCommunity(node);
      float weighted_degree = g->weightedDegree(node);
      
      // get the neighboring communities of the node
      auto neighboring_comms = neighboringCommunities(node);

      // remove the node from its community
      g->removeChild(node, neighboring_comms[node_comm]);

      // find the best community for node
      int best_comm = node_comm;
      double best_num_edges = 0.0, best_increase = 0.0;
      for (auto & ncd : neighboring_comms) {
        double increase = getGraph().modularityGain(node, ncd.first, ncd.second, weighted_degree);
        if (increase > best_increase) {
          best_comm = ncd.first;
          best_num_edges = ncd.second;
          best_increase = increase;
        }
      }

      // insert node in the best community
      g->addChild(best_comm, node, best_num_edges);
     
      if (best_comm != node_comm) {
        num_moves++;
      }
    }

    double prev_modularity = modularity;
    modularity = getGraph().modularity();
    if (num_moves > 0) {
      is_improved = true;
    }

    if (!num_moves || modularity - prev_modularity < min_modularity) {
      break;
    }    
  }

  return is_improved;
}
