#include "LouvainSimplifier.h"

#include <Graph.h>
#include <Louvain.h>
#include <iostream>

using namespace std;

bool
LouvainSimplifier::updateData(Graph & target_graph, time_t start_time, time_t end_time, float start_sentiment, float end_sentiment, Graph & source_graph, RawStatistics & stats, bool is_first_level, Graph * base_graph) {
  if (target_graph.getNodeArray().hasTemporalCoverage() && !(end_time > start_time)) {
    cerr << "invalid time range for updateData: " << start_time << " - " << end_time << endl;
    return false;
  }
  
  assert(base_graph);
  
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
    target_graph.clear();
    current_pos = 0;
    cerr << "restarting update, begin = " << begin.get() << ", cp = " << current_pos << ", end = " << end.get() << ", source = " << &source_graph << ", edges = " << source_graph.getEdgeCount() << endl;
  } else {
    cerr << "continuing update, begin = " << begin.get() << ", cp = " << current_pos << ", end = " << end.get() << ", source = " << &source_graph << ", edges = " << source_graph.getEdgeCount() << endl;
    for (int i = 0; i < current_pos; i++) ++it;
  }

  auto & nodes = target_graph.getNodeArray();

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
      assert(it->face >= 0 && it->face < source_graph.getFaceCount());
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
      
      auto td1 = base_graph->getNodeTertiaryData(np.first); // data is copied, since the backing array might change
      auto td2 = base_graph->getNodeTertiaryData(np.second);
      
      if (!is_first_level) {
	if (td1.indegree < target_graph.getMinSignificance()) {
	  skipped_count++;
	  continue;
	}
	if (td2.indegree < target_graph.getMinSignificance()) {
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
	UserType ut = UserType(user_type.getInt(np.second));
	if (ut != UNKNOWN_TYPE) stats.addUserType(ut);
	// stats.addPoliticalParty(PoliticalParty(political_party.getInt(np.first)));
      }

      if (target_type == NODE_HASHTAG) {
	stats.addHashtag(name_column.getText(np.second));
	num_hashtags++;
      } else if (target_type == NODE_URL) {
	stats.addLink(name_column.getText(np.second), uname_column.getText(np.second));
	num_links++;
      } else {
	if (is_first_level && target_type == NODE_ANY) {
	  stats.addReceivedActivity(t, target_user_sid, target_user_soid, app_id, filter_id);
	}

	long long coverage = 0;
	if (target_graph.getNodeArray().hasTemporalCoverage()) {
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
	  if (target_graph.getNodeArray().hasTemporalCoverage()) {
	    assert(0);
	    auto & ed = target_graph.getEdgeAttributes(it2->second);
	    ed.coverage |= coverage;
	    float new_weight = 0;
	    for (int i = 0; i < 64; i++) {
	      if (ed.coverage & (1 << i)) new_weight += 1.0f;
	    }
	    new_weight /= 64.0f;
	    target_graph.updateEdgeWeight(it2->second, new_weight - ed.weight);
	  }
	} else {	  
	  seen_edges[np.first][np.second] = target_graph.addEdge(np.first, np.second, -1, 1.0f / 64.0f, 0, target_graph.getNodeArray().hasTemporalCoverage() ? coverage : 1.0f);
#if 0
	  seen_edges[np.second][np.first] = target_graph.addEdge(np.second, np.first, -1, 1.0f / 64.0f, 0, target_graph.getNodeArray().hasTemporalCoverage() ? coverage : 1.0f);
#endif
	}
      }
    }  
  }

  // cerr << "updated graph data, nodes = " << nodes.size() << ", edges = " << getEdgeCount() << ", min_sig = " << target_graph.getMinSignificance() << ", skipped = " << skipped_count << ", first = " << is_first_level << endl;

  if (is_first_level) {
    stats.setTimeRange(min_time, max_time);
    stats.setNumRawNodes(nodes.size());
    stats.setNumRawEdges(source_graph.getEdgeCount());
    // stats.setNumPosts(num_posts);
    // stats.setNumActiveUsers(num_active_users);
  }

  if (is_changed) {
    target_graph.removeAllChildren();
    
    double precision = 0.000001;

    cerr << "doing Louvain\n";
    
    Louvain c(&target_graph, -1, precision);
    double mod = target_graph.modularity();
    int level = 0;
    bool is_improved = true;
    bool is_first = true;
    for (int level = 1; level <= 1 && is_improved; level++) {
      is_improved = c.oneLevel();
      double new_mod = target_graph.modularity();
      level++;
      
      cerr << "l " << level << ": size: " << c.size() << ", modularity increase: " << mod << " to " << new_mod << endl;
      mod = new_mod;
    }

    unsigned int visible_nodes = 0, toplevel_nodes = 0;
    auto end = target_graph.end_visible_nodes();
    for (auto it = target_graph.begin_visible_nodes(); it != end; ++it) {
      visible_nodes++;
      auto & td = target_graph.getNodeTertiaryData(*it);
      if (td.parent_node == -1) {
	toplevel_nodes++;
	if (td.child_count == 1) {
	  // target_graph.removeChild(td.first_child);
	}
	int best_d = 0;
	int best_node = -1;
	for (int n = td.first_child; n != -1; ) {
	  auto & ctd = target_graph.getNodeTertiaryData(n);
	  if (best_node == -1 || ctd.indegree > best_d) {
	    best_node = n;
	    best_d = ctd.indegree;
	  }
	  target_graph.setIsGroupLeader(n, false);
	  n = ctd.next_child;
	}
	if (best_node != -1) {
	  target_graph.setIsGroupLeader(best_node, true);
	}
      }
    }
    cerr << "after louvain: visible = " << visible_nodes << ", toplevel = " << toplevel_nodes << endl;
  }

  return is_changed;
}
