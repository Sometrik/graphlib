#include "DirectedGraph.h"

#include <cassert>
#include <iostream>

#include "RawStatistics.h"

using namespace std;

DirectedGraph::DirectedGraph(int _id) : Graph(_id) {

}

DirectedGraph::DirectedGraph(const DirectedGraph & other) : Graph(other) {
  
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

void
DirectedGraph::breakNodePair(int node_id) {
  auto it = node_pairs.find(node_id);
  if (it != node_pairs.end()) {
    removeChild(node_id);
    int other_id = it->second;
    node_pairs.erase(it);

    auto it_b = onedegree_nodes.find(node_id);
    if (it_b != onedegree_nodes.end()) {
      onedegree_nodes.erase(it_b);
    }
    
    auto it2 = node_pairs.find(other_id);
    assert(it2 != node_pairs.end());
    if (it2 != node_pairs.end()) {
      removeChild(other_id);
      node_pairs.erase(it2);

      auto it2_b = onedegree_nodes.find(other_id);
      if (it2_b != onedegree_nodes.end()) {
	onedegree_nodes.erase(it2_b);
      } else {
	// if a pair is broken, we add the other node to first one's group
	int o = getNodeArray().getOneDegreeNode(node_id);
	addChild(o, other_id);
	onedegree_nodes[other_id] = node_id;
      }
    }
  }
}

void
DirectedGraph::breakOneDegreeNode(int node_id) {
  breakNodePair(node_id);
  
  auto it = onedegree_nodes.find(node_id);
  if (it != onedegree_nodes.end()) {
    removeChild(node_id);
    onedegree_nodes.erase(it);
  }
}

void
DirectedGraph::breakZeroDegreeNode(int node_id) {
  auto it = zerodegree_nodes.find(node_id);
  if (it != zerodegree_nodes.end()) {
    removeChild(node_id);
    zerodegree_nodes.erase(it);
  }
}

bool
DirectedGraph::updateData(time_t start_time, time_t end_time, float start_sentiment, float end_sentiment, Graph & source_graph, RawStatistics & stats, bool is_first_level, Graph * base_graph) {
  if (getNodeArray().hasTemporalCoverage() && !(end_time > start_time)) {
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
	if (getNodeArray().hasTemporalCoverage()) {
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
	  if (getNodeArray().hasTemporalCoverage()) {
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
	    if (np.first != np.second) {
	      if (td2.indegree == 0 && td2.outdegree == 0) {
		breakZeroDegreeNode(np.second);
		assert(node_pairs.find(np.first) == node_pairs.end());
		assert(node_pairs.find(np.second) == node_pairs.end());
		int o = nodes.getPairsNode();
		cerr << "adding to pairs node\n";
		addChild(o, np.first);
		addChild(o, np.second);
		node_pairs[np.first] = np.second;
		node_pairs[np.second] = np.first;
	      } else if (!onedegree_nodes.count(np.first)) {
		cerr << "adding to onedegree node (A)\n";
		assert(node_pairs.find(np.first) == node_pairs.end());
		breakOneDegreeNode(np.second);
		breakNodePair(np.second);
		int o = nodes.getOneDegreeNode(np.second);
		addChild(o, np.first);
		onedegree_nodes[np.first] = np.second;
	      }
	    }
	  } else {
	    breakNodePair(np.first);
	  }
	  if (np.first != np.second) {
	    if (td2.indegree == 0 && td2.outdegree == 0) {
	      breakZeroDegreeNode(np.second);	      
	      if (td1.indegree == 0 && td1.outdegree == 0) {
		// pair was created
	      } else if (!onedegree_nodes.count(np.second)) {
		if (node_pairs.count(np.second)) {
		  cerr << "ERROR!\n";
		} else {
		  cerr << "adding child " << np.second << " to onedegree node (B) [td1.i = " << td1.indegree << ", td1.o = " << td1.outdegree << "]\n";
		  assert(node_pairs.find(np.second) == node_pairs.end());
		  breakOneDegreeNode(np.first);
		  breakNodePair(np.first);
		  int o = nodes.getOneDegreeNode(np.first);
		  addChild(o, np.second);
		  onedegree_nodes[np.second] = np.first;
		}
	      }
	    } else {
	      breakNodePair(np.second);
	    }

	    auto od1 = onedegree_nodes.find(np.first), od2 = onedegree_nodes.find(np.second);
	    auto pd1 = node_pairs.find(np.first), pd2 = node_pairs.find(np.second);
	    bool have_same_base = od1 != onedegree_nodes.end() && od2 != onedegree_nodes.end() && od1->second == od2->second && ((pd1 == node_pairs.end() && pd2 == node_pairs.end()) || (pd1 != node_pairs.end() && pd2 != node_pairs.end() && pd1->second == np.second && pd2->second == np.first));
	    
	    if (have_same_base) {
	      node_pairs[np.first] = np.second;
	      node_pairs[np.second] = np.first;
	    } else {
	      if (td1.indegree != 0 || td1.outdegree != 0) {
		breakOneDegreeNode(np.first);
	      }
	      if (td2.indegree != 0 || td2.outdegree != 0) {
		breakOneDegreeNode(np.second);
	      }
	    }
	  }
	  seen_edges[np.first][np.second] = addEdge(np.first, np.second, -1, 1.0f / 64.0f, 0, getNodeArray().hasTemporalCoverage() ? coverage : 1.0f);
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
