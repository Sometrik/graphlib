#include "DirectedGraph.h"

#include <cassert>
#include <iostream>

#include "RawStatistics.h"

using namespace std;

DirectedGraph::DirectedGraph(int _id) : Graph(1, _id) {

}

DirectedGraph::DirectedGraph(const DirectedGraph & other) : Graph(other) {
  
}

std::shared_ptr<Graph>
DirectedGraph::createSimilar() const {
  std::shared_ptr<Graph> graph(new DirectedGraph(getId()));
  graph->setLocationGraphValid(false);
  graph->setTemporal(isTemporal());
  graph->setPersonality(getPersonality());
  graph->setHasTemporalCoverage(hasTemporalCoverage());
  graph->setHasTextures(hasTextures());
  graph->setDoubleBufferedVBO(hasDoubleBufferedVBO());
  graph->setLineWidth(getLineWidth());
  graph->setNodeArray(nodes);
  
  return graph;
}

bool
DirectedGraph::updateData(time_t start_time, time_t end_time, float start_sentiment, float end_sentiment, Graph & source_graph, RawStatistics & stats, bool is_first_level, Graph * base_graph) {
  if (hasTemporalCoverage() && !(end_time > start_time)) {
    cerr << "invalid time range for updateData: " << start_time << " - " << end_time << endl;
    return false;
  }
  
  auto & sid = source_graph.getNodeArray().getTable()["source"];
  auto & soid = source_graph.getNodeArray().getTable()["id"];
  auto & user_type = source_graph.getNodeArray().getTable()["type"];
  auto & political_party = source_graph.getNodeArray().getTable()["party"];
  auto & name_column = source_graph.getNodeArray().getTable()["name"];
  auto & uname_column = source_graph.getNodeArray().getTable()["uname"];

  auto begin = source_graph.begin_edges();
  auto end = source_graph.end_edges();
  auto it = begin;
  
  if (current_pos == -1) {
    clear();
    edge_attributes.clear();
    current_pos = 0;
    cerr << "restarting update, begin = " << begin.get() << ", cp = " << current_pos << ", end = " << end.get() << ", source = " << &source_graph << ", edges = " << source_graph.getEdgeCount() << endl;
  } else {
    cerr << "continuing update, begin = " << begin.get() << ", cp = " << current_pos << ", end = " << end.get() << ", source = " << &source_graph << ", edges = " << source_graph.getEdgeCount() << endl;
    for (int i = 0; i < current_pos; i++) ++it;
  }

  auto & nodes = getNodeArray();

  unsigned int skipped_count = 0;
  bool is_changed = false;
  unsigned int num_edges_processed = 0;
  for ( ; it != end; ++it, current_pos++) {
    num_edges_processed++;

    time_t t = 0;
    float se = 0;
    short lang = 0;
    long long app_id = -1, filter_id = -1;
    bool is_first = false;

    assert(it->face != -1);
    if (it->face != -1) {
      auto & fd = source_graph.getFaceAttributes(it->face);
      t = fd.timestamp;
      se = fd.sentiment;
      lang = fd.lang;
      app_id = fd.app_id;
      filter_id = fd.filter_id;
      is_first = fd.first_edge == current_pos;
    }

    if (it->tail < 0 || it->head < 0 || it->tail >= nodes.size() || it->head >= nodes.size()) {
      cerr << "invalid values: tail = " << it->tail << ", head = " << it->head << ", t = " << t << ", count = " << nodes.size() << ", n = " << num_edges_processed << endl;
      assert(0);
    }

    if ((!start_time || t >= start_time) && (!end_time || t < end_time) &&
	se >= start_sentiment && se <= end_sentiment) {
      if (t < min_time || min_time == 0) min_time = t;
      if (t > max_time) max_time = t;

      pair<int, int> np(it->tail, it->head);
      short first_user_sid = sid.getInt(np.first);
      short target_user_sid = sid.getInt(np.second);
      long long first_user_soid = soid.getInt64(np.first);
      long long target_user_soid = soid.getInt64(np.second);
      
      auto & td1 = base_graph->getNodeTertiaryData(np.first);
      auto & td2 = base_graph->getNodeTertiaryData(np.second);
      
      if (!is_first_level) {
	if (td1.indegree < getMinSignificance()) {
	  skipped_count++;
	  continue;
	}
	if (td2.indegree < getMinSignificance()) {
	  skipped_count++;
	  continue;
	}
      }

      is_changed = true;

      auto & target_nd_old = nodes.getNodeData(np.second);
      NodeType target_type = target_nd_old.type;

      if (is_first_level && is_first) {
	stats.addActivity(t, first_user_sid, first_user_soid, lang, app_id, filter_id, PoliticalParty(political_party.getInt(np.first)));
      }
      
      if (is_first_level && !seen_nodes.count(np.second)) {
	seen_nodes.insert(np.second);
	if (target_type == NODE_HASHTAG) {
	  stats.addHashtag(name_column.getText(np.second));
	  num_hashtags++;
	} else if (target_type == NODE_URL) {
	  stats.addLink(name_column.getText(np.second), uname_column.getText(np.second));
	  num_links++;
	} else {
	  UserType ut = UserType(user_type.getInt(np.second));
	  if (ut != UNKNOWN_TYPE) stats.addUserType(ut);
	  // stats.addPoliticalParty(PoliticalParty(political_party.getInt(np.first)));
	}
      }

      if (target_type != NODE_URL && target_type != NODE_HASHTAG) {
	if (is_first_level && target_type == NODE_ANY) {
	  stats.addReceivedActivity(t, target_user_sid, target_user_soid, app_id, filter_id);
	}

	long long coverage = 0;
	if (hasTemporalCoverage()) {
	  assert(end_time > start_time);
	  int time_pos = 63LL * (t - start_time) / (end_time - start_time);
	  assert(time_pos >= 0 && time_pos < 64);
	  coverage |= 1 << time_pos;
	}
	
	unordered_map<int, unordered_map<int, int> >::iterator it1;
	unordered_map<int, int>::iterator it2;
	if ((it1 = seen_edges.find(np.first)) != seen_edges.end() &&
	    (it2 = it1->second.find(np.second)) != it1->second.end()) {
#if 0
	  updateOutdegree(np.first, 1.0f);
	  updateIndegree(np.second, 1.0f);
	  updateNodeSize(np.first);
	  updateNodeSize(np.second);
#endif
	  if (hasTemporalCoverage()) {
	    auto & ed = getEdgeAttributes(it2->second);
	    ed.coverage |= coverage;
	    float new_weight = 0;
	    for (int i = 0; i < 64; i++) {
	      if (ed.coverage & (1 << i)) new_weight += 1.0f;
	    }
	    new_weight /= 64.0f;
	    updateEdgeWeight(it2->second, new_weight - ed.weight);
	  }
	} else {
	  if (td1.indegree == 0 && td1.outdegree == 0) {
	    bool has_zero = zerodegree_nodes.count(np.first) != 0;
	    if (np.first == np.second && !has_zero) {
	      int z = nodes.getZeroDegreeNode();
	      cerr << "DEBUG: adding node " << np.first << " to zero degree node (id = " << z << ")\n";
	      addChild(z, np.first);
	      zerodegree_nodes.insert(np.first);
	    } else if (np.first != np.second && has_zero) {
	      cerr << "DEBUG: removing node " << np.first << " from zero degree node (A)\n";
	      removeChild(np.first);
	      zerodegree_nodes.erase(np.first);
	    }
	    if (np.first != np.second && !onedegree_nodes.count(np.first)) {	      
	      int o = nodes.getOneDegreeNode(np.second);
	      addChild(o, np.first);
	      onedegree_nodes[np.first] = np.second;	      
	    }
	  }
	  if (td2.indegree == 0 && td2.outdegree == 0 && np.first != np.second) {
	    if (zerodegree_nodes.count(np.second) != 0) {
	      cerr << "DEBUG: removing node " << np.second << " from zero degree node (B)\n";
	      removeChild(np.second);
	      zerodegree_nodes.erase(np.second);
	    }
	    if (!onedegree_nodes.count(np.second)) {
	      int o = nodes.getOneDegreeNode(np.first);
	      addChild(o, np.second);
	      onedegree_nodes[np.second] = np.first;
	    }
	  }
	  if (np.first != np.second) {
	    auto it1 = onedegree_nodes.find(np.first);
	    auto it2 = onedegree_nodes.find(np.second);
	    if (it1 != onedegree_nodes.end() && np.second != it1->second && (td1.indegree != 0 || td2.outdegree != 0)) {
	      removeChild(np.first);
	      onedegree_nodes.erase(it1);
	    }
	    if (it2 != onedegree_nodes.end() && np.first != it2->second && (td2.indegree != 0 || td2.outdegree != 0)) {
	      removeChild(np.second);
	      onedegree_nodes.erase(it2);	      
	    }
	  }
	  seen_edges[np.first][np.second] = addEdge(np.first, np.second, -1, 1.0f / 64.0f, 0, hasTemporalCoverage() ? coverage : 1.0f);
	}
      }
    }  
  }

  cerr << "updated graph data, nodes = " << nodes.size() << ", edges = " << getEdgeCount() << ", min_sig = " << getMinSignificance() << ", skipped = " << skipped_count << ", first = " << is_first_level << endl;

  if (is_first_level) {
    stats.setTimeRange(min_time, max_time);
    stats.setNumRawNodes(nodes.size());
    stats.setNumRawEdges(source_graph.getEdgeCount());
    // stats.setNumPosts(num_posts);
    // stats.setNumActiveUsers(num_active_users);
  }

  return is_changed;
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
