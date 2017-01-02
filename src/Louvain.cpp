#include "Louvain.h"

#include <Graph.h>

#include <iostream>
#include <cassert>
#include <unordered_set>

using namespace std;

Louvain::Louvain(Graph * _g, int _max_num_passes, double _min_modularity,
		 bool _create_node_clusters, bool _create_edge_clusters)
  : g(_g),
    max_num_passes(_max_num_passes),
    min_modularity(_min_modularity),
    create_node_clusters(_create_node_clusters),
    create_edge_clusters(_create_edge_clusters)
{
}

int
Louvain::getNodeCommunity(int node) const {
  while ( 1 ) {
    auto & td = getGraph().getNodeTertiaryData(node);
    if (td.parent_node == -1) break;
    node = td.parent_node;
  }
  return node;
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
      if (it == comms.end()) {
	comms[neigh_comm] = neigh_w;
      } else {
	it->second += neigh_w;
      }      
    }
  }

  return comms;
}

bool
Louvain::oneLevel() {
  std::vector<int> nodes, edges;

  if (create_node_clusters) {
    auto end = g->end_visible_nodes();
    for (auto it = g->begin_visible_nodes(); it != end; ++it) {
      // if (getNodeCommunity(*it) == -1) {
      if (getGraph().getNodeTertiaryData(*it).parent_node == -1) {
	nodes.push_back(*it);
      }
    }
    for (auto & n : nodes) {
      int community_id = g->getNodeArray().createCommunity(n);
      // g->getNodeArray().setPosition(community_id, g->getNodePosition(n));
      g->addChild(community_id, n, 0);
      // g->getNodeArray().setPosition(n, glm::vec3());
    }
  } else {
    assert(0);
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
  
  bool is_improved = false;
  double modularity = getGraph().modularity();

#if 0
  // shuffle nodes
  for (int i = 0; i < nodes.size()-1; i++) {
    int rand_pos = rand() % (nodes.size()-i)+i;
    int tmp = nodes[i];
    nodes[i] = nodes[rand_pos];
    nodes[rand_pos] = tmp;
  }
#endif

  // cerr << "max_num_passes = " << max_num_passes << endl;
  
  // repeat until there is no improvement in modularity, or the improvement is smaller than epsilon, or maximum number of passes have been done  
  for (int num_passes = 0; max_num_passes == -1 || num_passes < max_num_passes; num_passes++) {
    int num_moves = 0;

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
      
    if (!num_moves) {
      cerr << "Louvain: stopping due to no moves\n";
      break;
    } if (modularity - prev_modularity < min_modularity) {
      cerr << "Louvain: modularity diff too small, min = " << min_modularity << "\n";
      break;
    }
  }

  std::unordered_set<int> clusters;

  if (is_improved) {
    for (int node : nodes) {
      clusters.insert(getNodeCommunity(node));
    }
    
    current_clusters.clear();
    current_clusters.reserve(clusters.size());
    for (auto id : clusters) {
      current_clusters.push_back(id);
    }
  } else {
    for (int node : nodes) {
      getGraph().removeChild(node);
    }
  }
  
  return is_improved;
}
