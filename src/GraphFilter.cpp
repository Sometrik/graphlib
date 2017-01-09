#include <GraphFilter.h>

#include <Graph.h>

#include <iostream>

// Hashtag weight should be at least little larger than URL weight
// since we prefer to have a hashtag as a representative node

#define ATTRIBUTE_WEIGHT	0.15f
#define HASHTAG_WEIGHT		0.26f
#define URL_WEIGHT		0.24f

using namespace std;

bool
GraphFilter::processTemporalData(Graph & target_graph, time_t start_time, time_t end_time, float start_sentiment, float end_sentiment, Graph & source_graph, RawStatistics & stats) {  
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
    // cerr << "restarting update, begin = " << begin.get() << ", cp = " << current_pos << ", end = " << end.get() << ", source = " << &source_graph << ", edges = " << source_graph.getEdgeCount() << endl;
  } else {
    // cerr << "continuing update, begin = " << begin.get() << ", cp = " << current_pos << ", end = " << end.get() << ", source = " << &source_graph << ", edges = " << source_graph.getEdgeCount() << endl;
    for (int i = 0; i < current_pos; i++) {
      assert(it != end);
      ++it;
    }
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
      is_first = fd.first_edge == current_pos;

      auto filter_column = source_graph.getFaceData().getColumnSafe("filterId");
      if (filter_column) filter_id = filter_column->getInt64(it->face);
    }

    if (it->tail < 0 || it->head < 0 || it->tail >= nodes.size() || it->head >= nodes.size()) {
      cerr << "GraphFilter: invalid values: tail = " << it->tail << ", head = " << it->head << ", t = " << t << ", count = " << nodes.size() << ", n = " << num_edges_processed << endl;
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
      
      is_changed = true;

      auto & target_nd_old = nodes.getNodeData(np.second);
      NodeType target_type = target_nd_old.type;

      bool is_newly_active1 = target_graph.getNodeTertiaryData(np.first).outdegree == 0;
      bool is_new_node1 = !target_graph.isNodeVisible(np.first);
      bool is_new_node2 = !target_graph.isNodeVisible(np.second);
      
      if (is_first) {
	stats.addActivity(t, first_user_sid, first_user_soid, lang, app_id, filter_id, PoliticalParty(political_party.getInt(np.first)));

	if (lang && keep_lang) {
	  int lang_node = nodes.createLanguage(lang);
	  if (!target_graph.hasEdge(np.first, lang_node)) {
	    target_graph.addEdge(np.first, lang_node, -1, ATTRIBUTE_WEIGHT);
	  }
	}
	if (app_id > 0 && keep_applications) {
	  int app_node = nodes.createApplication(app_id);
	  if (!target_graph.hasEdge(np.first, app_node)) {
	    target_graph.addEdge(np.first, app_node, -1, ATTRIBUTE_WEIGHT);
	  }
	}
      }

      if (is_newly_active1 || is_new_node1) {
	UserType ut = UserType(user_type.getInt(np.first));
	
	if (is_newly_active1) {
	  if (ut != UNKNOWN_TYPE) stats.addUserType(ut);
	  // stats.addPoliticalParty(PoliticalParty(political_party.getInt(np.first)));
	}

	if (is_new_node1) {
	  int type_node_id = -1;
	  if (ut == MALE) type_node_id = nodes.createMaleNode();
	  else if (ut == FEMALE) type_node_id = nodes.createFemaleNode();
	  if (type_node_id != -1 && !target_graph.hasEdge(np.first, type_node_id)) {
	    target_graph.addEdge(np.first, type_node_id, -1, ATTRIBUTE_WEIGHT);
	  }
	}
      }
      
      if (is_new_node2) {
	UserType ut = UserType(user_type.getInt(np.second));
	int type_node_id = -1;
	if (ut == MALE) type_node_id = nodes.createMaleNode();
	else if (ut == FEMALE) type_node_id = nodes.createFemaleNode();
	if (type_node_id != -1 && !target_graph.hasEdge(np.second, type_node_id)) {
	  target_graph.addEdge(np.second, type_node_id, -1, ATTRIBUTE_WEIGHT);
	}
      }

      float weight = 1.0f;
      if (target_type == NODE_ANY) {
	stats.addReceivedActivity(t, target_user_sid, target_user_soid, app_id, filter_id);
      } else if (target_type == NODE_HASHTAG) {
	stats.addHashtag(name_column.getText(np.second));
	num_hashtags++;
	weight = HASHTAG_WEIGHT;
      } else if (target_type == NODE_URL || target_type == NODE_IMAGE) {
	stats.addLink(name_column.getText(np.second), uname_column.getText(np.second));
	num_links++;
	weight = URL_WEIGHT;
      }
      
      if ((keep_hashtags || target_type != NODE_HASHTAG) &&
	  (keep_links || (target_type != NODE_URL && target_type != NODE_IMAGE)) &&
	  !target_graph.hasEdge(np.first, np.second)) {
	target_graph.addEdge(np.first, np.second, -1, weight);
      }
    }  
  }

  // cerr << "updated graph data, nodes = " << nodes.size() << ", edges = " << getEdgeCount() << ", min_sig = " << target_graph.getMinSignificance() << ", skipped = " << skipped_count << ", first = " << is_first_level << endl;

  stats.setTimeRange(min_time, max_time);
  stats.setNumRawNodes(nodes.size());
  stats.setNumRawEdges(source_graph.getEdgeCount());
  // stats.setNumPosts(num_posts);
  // stats.setNumActiveUsers(num_active_users);

  return is_changed;
}
