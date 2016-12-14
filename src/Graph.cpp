#include "Graph.h"

#include <StringUtils.h>

#include "DisplayInfo.h"
#include "ColorProvider.h"
#include "Controller.h"
#include "RenderMode.h"
#include "Label.h"

#include <DateTime.h>
#include <algorithm>
#include <iostream>
#include <typeinfo>
#include <unordered_set>

#include "Louvain.h"

#define EPSILON 0.0000000001
#define INITIAL_ALPHA		0.1f

using namespace std;

int Graph::next_id = 1;

Graph::Graph(int _id)
  : id(_id),
    node_color(0.0f, 0.0f, 0.0f, 0.0f),
    edge_color(0.0f, 0.0f, 0.0f, 0.0f),
    face_color(0.0f, 0.0f, 0.0f, 0.0f)
{
  if (!id) id = next_id++;
}


Graph::~Graph() {
  
}

Graph::Graph(const Graph & other)
  : nodes(other.nodes), // copy node array by reference
    faces(other.faces),
    face_attributes(other.face_attributes),
    edge_attributes(other.edge_attributes),
    version(other.version),
    source_id(other.source_id)
{
  id = next_id++;
}
 
bool
Graph::hasEdge(int n1, int n2) const {
  int edge = getNodeFirstEdge(n1);
  while (edge != -1) {
    if (getEdgeTargetNode(edge) == n2) {
      return true;
    }
    edge = getNextNodeEdge(edge);
  }
  return false;
}

static table::Column * sort_col = 0;

static bool compareRows(const int & a, const int & b) {
  return sort_col->compare(a, b);
}

#if 0
void
Graph::setFaceColorByColumn(int column) {
  sort_col = &(regions[column]);
  cerr << "setting colors by column " << sort_col->name() << endl;
  vector<int> v;
  for (int i = 0; i < getRegionCount(); i++) {
      v.push_back(i);
  }
  sort(v.begin(), v.end(), compareRows);
  glm::vec3 c1(0.5f, 0.1f, 0.9f), c2(0.05f, 0.01f, 0.09f);
  for (int i = 0; i < v.size(); i++) {
    cerr << "region " << i << ": " << sort_col->getDouble(v[i]) << endl;
#if 1
    glm::vec3 c = glm::normalize(glm::mix(c1, c2, (float)(v.size() - 1 - i) / (v.size() - 1)));
#else
    Colorf c;
    c.setHSL(2.0f * (v.size() - 1 - i) / (v.size() - 1) / 3.0f, 0.75, 0.5);
#endif
    region_attributes[v[i]].color = { (unsigned char)glm::clamp(int(c.r * 255.0f), 0, 255),
				      (unsigned char)glm::clamp(int(c.g * 255.0f), 0, 255),
				      (unsigned char)glm::clamp(int(c.b * 255.0f), 0, 255),
				      255
    };

  }
  version++;
}
#endif

void
Graph::randomizeGeometry(bool use_2d) {
  assert(!nodes->hasSpatialData());
  unsigned int num_nodes = getNodeArray().size();
  for (unsigned int i = 0; i < num_nodes; i++) {
    getNodeArray().setRandomPosition(i, use_2d);
  }
  for (auto & gd : nested_graphs) {
    auto & graph = gd.second;
    if (!graph->nodes->hasSpatialData()) {
      graph->randomizeGeometry(use_2d);       
    }
  }
  mbr = Rect2d(-50, -50, +50, +50);
}

void
Graph::randomizeChildGeometry(int node_id, bool use_2d) {
  assert(!nodes->hasSpatialData());
  assert(node_id >= 0);
  if (node_id < node_geometry3.size()) {
    auto & td = node_geometry3[node_id];
    for (int c = td.first_child; c != -1; ) {
      getNodeArray().setRandomPosition(c, use_2d);
      auto & td2 = node_geometry3[c];
      c = td2.next_child;
    }
  }
}

void
Graph::getVisibleLabels(vector<Label> & labels) const {  
  const table::Column & user_type = getNodeArray().getTable()["type"];
  auto & size_method = nodes->getNodeSizeMethod();
  
  glm::vec4 black(0.0, 0.0, 0.0, 1.0), white(1.0, 1.0, 1.0, 1.0);
  
  for (unsigned int i = 0; i < getFaceCount(); i++) {
    auto & fd = getFaceAttributes(i);
    if (!(fd.flags & FACE_LABEL_VISIBLE && fd.label_texture)) continue;

    glm::vec3 pos(fd.centroid.x, fd.centroid.y, 0.0f);

    unsigned short flags = 0;
    flags |= LABEL_FLAG_MIDDLE;

    labels.push_back({ pos, glm::vec2(), fd.label_texture, flags, black, white });
  }

  vector<Label> primary_labels;
  
  auto nodes_end = end_visible_nodes();
  for (auto it = begin_visible_nodes(); it != nodes_end; ++it) {
    auto & pd = getNodeArray().getNodeData(*it);
    auto & td = getNodeTertiaryData(*it);
    if (!(td.isLabelVisible() && pd.label_texture)) continue;

    auto pos = pd.position;
    bool parent_visible = true;				 
    for (int p = td.parent_node; p != -1; ) {
      auto & ptd = getNodeTertiaryData(p);
      if (p != active_child_node) {
	pos = getNodeArray().getNodeData(p).position;
	parent_visible = false;
      } else {
	pos += getNodeArray().getNodeData(p).position;
      }
      p = ptd.parent_node;
    }
    
    glm::vec2 offset;
    unsigned short flags = 0;

    glm::vec4 color1 = black, color2 = white;
    if (td.child_count) {
      float size = size_method.calculateSize(td, total_indegree, total_outdegree, nodes->size());
      color1 = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
      // offset += glm::vec2(0, 0.5 * size);
      pos += glm::vec3(0.0, size, 0.0);
      flags |= LABEL_FLAG_CENTER;
      flags |= LABEL_FLAG_MIDDLE;

      labels.push_back({ pos, offset, pd.label_texture, flags, color1, color2 });
    } else if (pd.type == NODE_URL || pd.type == NODE_HASHTAG) {
      color1 = glm::vec4(0.0f, 0.0f, 0.0f, 0.25f);
      flags |= LABEL_FLAG_MIDDLE;
      flags |= LABEL_FLAG_CENTER;
      labels.push_back({ pos, offset, pd.label_texture, flags, color1, color2 });
    } else if (getNodeArray().getLabelStyle() == LABEL_DARK_BOX) {
      float size = size_method.calculateSize(td, total_indegree, total_outdegree, nodes->size());
      offset += glm::vec2(0.0f, -3.2f * size);
      flags |= LABEL_FLAG_MIDDLE;
      flags |= LABEL_FLAG_CENTER;

      primary_labels.push_back({ pos, offset, pd.label_texture, flags, color1, color2 });
    }        
  }
  
  labels.insert(labels.end(), primary_labels.begin(), primary_labels.end());
}

unsigned int
Graph::calcVisibleNodeCount() const {
  unsigned int n = 0;
  auto nodes_end = end_visible_nodes();
  for (auto it = begin_visible_nodes(); it != nodes_end; ++it) {
    n++;
  }
  return n;
}

// Gauss-Seidel relaxation for links
void
Graph::relaxLinks(std::vector<node_position_data_s> & v) const {
  unsigned int visible_nodes = calcVisibleNodeCount();
  double avg_edge_weight = total_edge_weight / getEdgeCount();
  float alpha = getNodeArray().getAlpha();
  auto & size_method = nodes->getNodeSizeMethod();
  bool flatten = nodes->doFlattenHierarchy();
  unsigned int num_nodes = nodes->size();
  float max_idf = log(visible_nodes / 1.0f);
  vector<bool> processed_edges;  
  processed_edges.resize(num_nodes * num_nodes);			
  auto end = end_edges();
  for (auto it = begin_edges(); it != end; ++it) {
    if (it->weight < 1.0f) continue;
    int tail = it->tail, head = it->head;
    int level = 0;
    assert(tail >= 0 && head >= 0);
    if (flatten) {
      int l1 = 0, l2 = 0;
      if (tail < node_geometry3.size()) {
        for (int p = node_geometry3[tail].parent_node; p != -1; p = node_geometry3[p].parent_node) l1++;
      }
      if (head < node_geometry3.size()) {
        for (int p = node_geometry3[head].parent_node; p != -1; p = node_geometry3[p].parent_node) l2++;
      }
      while ( 1 ) {
	if (l1 > l2) {
	  tail = node_geometry3[tail].parent_node;
	  l1--;
	} else if (l2 > l1) {
	  head = node_geometry3[head].parent_node;
	  l2--;
	} else if (l1 == 0 || l2 == 0 || node_geometry3[tail].parent_node == node_geometry3[head].parent_node) {
	  break;
	} else {
	  l1--; l2--;
	  tail = node_geometry3[tail].parent_node;
	  head = node_geometry3[head].parent_node;
	}
      }
      unsigned int key = tail * num_nodes + head;
      assert(key < processed_edges.size());
      if (processed_edges[key]) continue;
      processed_edges[key] = true;
      level = l1;
    }
    if (tail == head || (it->weight > -EPSILON && it->weight < EPSILON)) continue;
    bool visible = true;

    auto & td1 = getNodeTertiaryData(tail), & td2 = getNodeTertiaryData(head);

    bool open = false;
    if (getNodeTertiaryData(tail).parent_node == active_child_node &&
	getNodeTertiaryData(head).parent_node == active_child_node) {
      open = true;
    }
    if (!open) continue;
    
    bool fixed1 = td1.isFixed();
    bool fixed2 = td2.isFixed();
    if (fixed1 && fixed2) continue;

    auto & pd1 = v[tail], & pd2 = v[head];
    glm::vec3 & pos1 = pd1.position, & pos2 = pd2.position;
    glm::vec3 d = pos2 - pos1;
    float l = glm::length(d);

    if (l < EPSILON) continue;

    d *= 1 / l;
      
    float w1 = size_method.calculateSize(td1, total_indegree, total_outdegree, nodes->size());
    float w2 = size_method.calculateSize(td2, total_indegree, total_outdegree, nodes->size());

    if (td1.child_count) l -= w1;
    if (td2.child_count) l -= w2;

    if (l < EPSILON) continue;

    assert(td1.parent_node == td2.parent_node);

    float degree1 = td1.outdegree, degree2 = td2.indegree;
    float degree = degree1 > degree2 ? degree1 : degree2;
    if (degree == 0) degree = 1;
    float idf = 1.0f; // log(visible_nodes / degree) / max_idf;
    // float a = idf > 1.0 ? 1.0 : idf;
    
    // d *= getAlpha() * it->weight * link_strength * (l - link_length) / l;
    // d *= alpha * fabsf(it->weight) / max_edge_weight; // / avg_edge_weight;
    l *= (level == 0 ? alpha : alpha / 48.0f) * idf;
    // l *= alpha * idf;
    
    float k;
    if (fixed1) {
      k = 1.0f;
    } else if (fixed2) {
      k = 0.0f;
    } else {
      k = w1 / (w1 + w2);
    }
    pos2 -= d * l * k;
    pos1 += d * l * (1 - k);
  }
}

int
Graph::getFaceId(short source_id, long long source_object_id) const {
  auto it = getFaceCache().find(skey(source_id, source_object_id));
  if (it != getFaceCache().end()) {
    return it->second;
  }
  return -1;
}

int
Graph::pickNode(const DisplayInfo & display, int x, int y, float node_scale) const {
  int best_i = -1;
  float best_d = 0;
  glm::vec2 ppos(x, y);
  auto & size_method = nodes->getNodeSizeMethod();
    
  auto end = end_visible_nodes();
  for (auto it = begin_visible_nodes(); it != end; ++it) {
    auto & pd = nodes->getNodeData(*it);
    auto & td = getNodeTertiaryData(*it);
    float size = size_method.calculateSize(td, total_indegree, total_outdegree, nodes->size());
    auto pos = pd.position;
    for (int p = td.parent_node; p != -1; p = getNodeTertiaryData(p).parent_node) {
      pos += nodes->getNodeData(p).position;
    }
    glm::vec3 tmp1 = display.project(pos);
    glm::vec3 tmp2 = display.project(pos + glm::vec3(size / 2.0f / node_scale, 0.0f, 0.0f));
    glm::vec2 pos1(tmp1.x, tmp1.y);
    glm::vec2 pos2(tmp2.x, tmp2.y);
    glm::vec2 tmp3 = pos2 - pos1;
    float diam = glm::length(tmp3);
    pos1 -= ppos;
    float d = glm::length(pos1) - diam;
    if (d <= 0 && (best_i == -1 || d < best_d)) {
      best_i = *it;
      best_d = d;
    }
  }
  return best_i;
}

void
Graph::extractLocationGraph(Graph & target_graph) {
  map<string, int> node_mapping;
  target_graph.getNodeArray().getTable().addTextColumn("name");

  // Column & sentiment = target_graph.getNodeArray().getNodeArray().getTable().addDoubleColumn("sentiment");

  const table::Column & lat_column = getNodeArray().getTable()["latitude"], & lon_column = getNodeArray().getTable()["longitude"];  
 
  map<int, map<int, int> > seen_edges;
  // map<int, pair<float, unsigned int> > node_sentiments;
  
  auto end = end_edges();
  for (auto it = begin_edges(); it != end; ++it) {
    // float edge_sentiment = it->sentiment;
    pair<int, int> np(it->tail, it->head);

    double lon1 = lon_column.getDouble(np.first), lat1 = lat_column.getDouble(np.first);
    double lon2 = lon_column.getDouble(np.second), lat2 = lat_column.getDouble(np.second);
    
    if (lon1 == 0 && lat1 == 0) {
      np.first = -1;
    } else {
      string key1 = to_string(lon1) + "/" + to_string(lat1);
            
      auto it = node_mapping.find(key1);
      if (it != node_mapping.end()) {
	np.first = it->second;
      } else {
	int new_node_id = target_graph.addNode();
	node_mapping[key1] = new_node_id;
	glm::vec3 tmp((float)lon1, (float)lat1, 0);
	target_graph.getNodeArray().setPosition(new_node_id, tmp);
	np.first = new_node_id;
      }

      // pair<float, unsigned int> & sn = node_sentiments[np.first];
      // sn.first += edge_sentiment;
      // sn.second++;
      
      NodeType type = getNodeArray().getNodeData(np.second).type;
      assert(type != NODE_URL && type != NODE_HASHTAG);
    }
    
    if (lon2 == 0 && lat2 == 0) {
      np.second = -1;
    } else {
      string key2 = to_string(lon2) + "/" + to_string(lat2);
      auto it = node_mapping.find(key2);
      if (it != node_mapping.end()) {
	np.second = it->second;
      } else {
	int new_node_id = target_graph.addNode();
	node_mapping[key2] = new_node_id;
	glm::vec3 tmp((float)lon2, (float)lat2, 0);
	target_graph.getNodeArray().setPosition(new_node_id, tmp);
	np.second = new_node_id;
      }
    }

    if (np.first >= 0 && np.second >= 0 && np.first != np.second) {
      map<int, map<int, int> >::iterator it1;
      map<int, int>::iterator it2;
      if ((it1 = seen_edges.find(np.first)) != seen_edges.end() &&
	  (it2 = it1->second.find(np.second)) != it1->second.end()) {
	int edge = it2->second;
	target_graph.getEdgeAttributes(edge).weight += 1.0f;
	// weight.setValue(edge, weight.getInt(edge) + 1);
      } else {
	int edge = seen_edges[np.first][np.second] = target_graph.addEdge(np.first, np.second);	  
	// weight.setValue(edge, 1);
      }
    }
  }
  
  target_graph.getNodeArray().setSRID(4326);
  target_graph.getNodeArray().setNodeSizeMethod(getNodeArray().getNodeSizeMethod());
  target_graph.setNodeVisibility(true);
  target_graph.setEdgeVisibility(true);
  target_graph.setFaceVisibility(false);
  target_graph.setLabelVisibility(true);  
}

static bool compareCameraDistance(const pair<int, float> & a, const pair<int, float> & b) {
  return a.second > b.second;
}

void
Graph::selectNodes(int input_node, int depth) {
  if (input_node == -1) {
    has_node_selection = false;
    for (int n = 0; n < node_geometry3.size(); n++) {
      node_geometry3[n].flags |= NODE_IS_SELECTED;
    }
  } else {
    has_node_selection = true;
    for (int n = 0; n < node_geometry3.size(); n++) {
      node_geometry3[n].flags &= ~NODE_IS_SELECTED;
    }
    
    if (input_node >= 0) { // can be -2 to select none
      map<int, set<int> > all_edges;
      
      auto end = end_edges();
      for (auto it = begin_edges(); it != end; ++it) {
	all_edges[it->tail].insert(it->head);
	all_edges[it->head].insert(it->tail);
      }
      
      set<int> seen_nodes;
      list<pair<int, int> > node_queue;
      node_queue.push_back(make_pair(input_node, 0));
      
      while (!node_queue.empty()) {
	int node_id = node_queue.front().first;
	int node_depth = node_queue.front().second;
	node_queue.pop_front();
	
	if (!seen_nodes.count(node_id)) {
	  seen_nodes.insert(node_id);

	  if (node_geometry3.size() <= node_id) node_geometry3.resize(node_id + 1);
	  node_geometry3[node_id].flags |= NODE_IS_SELECTED;
	  
	  if (node_depth < depth) {
	    map<int, set<int> >::iterator node_edges = all_edges.find(node_id);
	    if (node_edges != all_edges.end()) {
	      for (auto succ : node_edges->second) {
		node_queue.push_back(make_pair(succ, node_depth + 1));
	      }
	    }
	  }
	}
      }
    }
  }
}

std::vector<int>
Graph::getNestedGraphIds() const {
  std::vector<int> v;
  for (auto & gd : nested_graphs) {
    v.push_back(gd.second->getId());
  }
  return v;
}

std::vector<int>
Graph::getLocationGraphs() const {
  std::vector<int> v;
  for (auto & gd : nested_graphs) {
    auto & graph = gd.second;
    if (graph->getLocation().get()) v.push_back(graph->getId());
  }
  return v;
}

void
Graph::refreshLayouts() {
  cerr << "resume after refreshLayouts\n";
  getNodeArray().resume();
  for (auto & gd : nested_graphs) {
    gd.second->refreshLayouts();
  }
}

int
Graph::getGraphNodeId(int graph_id) const {
  for (auto & gd : nested_graphs) {
    if (gd.second->getId() == graph_id) return gd.first;
  }
  return -1;
}

void
Graph::createClusters() {
  cerr << "creating communities\n";

  double precision = 0.000001;
  ColorProvider colors(ColorProvider::CHART2);
    
  Louvain c(this, -1, precision);
  double mod = modularity();
  int level = 0;
  bool is_improved = true;
  bool is_first = true;
  do {
    is_improved = c.oneLevel();
    double new_mod = modularity();
    level++;
    
    cerr << "l " << level << ": modularity increase: " << mod << " to " << new_mod << endl;
    mod = new_mod;
    break;
  } while (is_improved);

  incVersion();
}

bool
Graph::updateSelection(time_t start_time, time_t end_time, float start_sentiment, float end_sentiment) {
  if (nodes->getPersonality() != NodeArray::SOCIAL_MEDIA || !nodes->isTemporal()) {
    return false;
  }

  if (!final_graph.get()) {
    cerr << "creating final graph\n";
    final_graph.reset();
    if (getFilter().get()) getFilter()->reset();
    auto g1 = createSimilar();
    assert(g1.get());
    setFinalGraph(g1);
    statistics.clear();
  } else if (getFilter().get() && !getFilter()->hasPosition()) {
    cerr << "RESETTING EVERYTHING!\n";
    if (getFilter().get()) getFilter()->reset();
    if (final_graph->hasNodeSelection()) {
      selectNodes(-2);
    } else {
      selectNodes();
    }
    
    statistics.clear();
  }
    
  statistics.setSentimentRange(start_sentiment, end_sentiment);
  
  bool changed = false;
  assert(final_graph.get());
  cerr << "trying to apply filter: st = " << int(start_time) << ", et = " << int(end_time) << ", ss = " << start_sentiment << ", es = " << end_sentiment << "\n";
  if (applyFilter(start_time, end_time, start_sentiment, end_sentiment)) {
    cerr << "filter changed the graph\n";
    final_graph->incVersion();
    setLocationGraphValid(false);
    assert(nodes->isDynamic());
    incVersion();
    getNodeArray().resume();
    final_graph->getNodeArray().resume();
    changed = true;
  }
  
  return changed;         
}

void
Graph::calculateEdgeCentrality() {
  unsigned int num_edges = getEdgeCount();
  vector<double> betweenness_data(num_edges, 0);

  cerr << "edge centrality: n = " << num_edges << endl;
  
  for (unsigned int source = 0; source < num_edges; source++) {
    vector<double> sigma_data(num_edges, 0);
    vector<double> delta_data(num_edges, 0);
    vector<int> distance_data(num_edges, -1);
    vector<list<int> > predecessors(num_edges, list<int>());
    
    sigma_data[source] = 1;
    distance_data[source] = 0;
    
    list<int> stack, queue;
    queue.push_back(source); 
   
    while (!queue.empty()) {
      int e = queue.front();
      queue.pop_front();
      stack.push_back(e);

      auto & ed = getEdgeAttributes(e);
      int target_node = ed.head;

      int succ = getNodeFirstEdge(target_node);
      while (succ != -1) {
	if (distance_data[succ] < 0) { // succ found for first time
	  queue.push_back(succ);
	  distance_data[succ] = distance_data[e] + 1;
	}
	if (distance_data[succ] == distance_data[e] + 1) { // shortest path to succ via v
	  sigma_data[succ] += sigma_data[e];
	  predecessors[succ].push_back(e);
	}
	succ = getNextNodeEdge(succ);
      }
    }
    
    for (list<int>::reverse_iterator it2 = stack.rbegin(); it2 != stack.rend(); it2++) {
      int w = *it2;
      for (list<int>::iterator it3 = predecessors[w].begin(); it3 != predecessors[w].end(); it3++) {
	int pre = *it3;
	delta_data[pre] += (sigma_data[pre] / sigma_data[w]) * (1 + delta_data[w]);
      }
      if (w != source) {
	betweenness_data[w] += delta_data[w];
      }
    }
  }
  
  // normalise & clean up
    
  double factor = (num_edges - 1) * (num_edges - 2);
  
  double sum = 0;
  total_edge_weight = 0;
  for (unsigned int i = 0; i < num_edges; i++) {
    sum += betweenness_data[i];
    double w = betweenness_data[i] == 0 ? 0.1 : 1 + log(1 + betweenness_data[i]);
    if (betweenness_data[i] != 0) total_edge_weight += w;
    getEdgeAttributes(i).weight = w;
  }

  total_edge_weight *= 4;

  cerr << "sum = " << sum << ", avg =  " << sum / num_edges << endl;

  assert(nodes->isDynamic());
  incVersion();
  randomizeGeometry();
  getNodeArray().resume();
}

struct label_data_s {
  enum { NODE, FACE } type;
  glm::vec3 world_pos;
  glm::vec2 screen_pos;
  float size, priority;
  int index;
};

static bool comparePriority(const label_data_s & a, const label_data_s & b) {
  if (a.priority != b.priority) {
    return a.priority < b.priority;
  } else {
    return a.size > b.size;
  }
}

bool
Graph::setActiveChildNode(int id) {
  if (id != active_child_node) {
    active_child_node = id;
    return true;
  } else {
    return false;
  }
}

bool
Graph::updateVisibilities(const DisplayInfo & display, bool reset) {
  vector<label_data_s> all_labels;
  auto & size_method = nodes->getNodeSizeMethod();
  auto & label_method = nodes->getLabelMethod();
  bool labels_changed = false, structure_changed = false;

  bool has_priority_column = !label_method.getPriorityColumn().empty();
  auto & node_priority_column = nodes->getTable()[label_method.getPriorityColumn()];
  auto & face_priority_column = getFaceData()[label_method.getPriorityColumn()];

  int best_child = -1;
  float best_score = 0.0f;
  
  auto end = end_visible_nodes();
  for (auto it = begin_visible_nodes(); it != end; ++it) {
    auto & pd = getNodeArray().getNodeData(*it);
    if (node_geometry3.size() <= *it) node_geometry3.resize(*it + 1);
    auto & td = node_geometry3[*it];
    if (td.age < 0.0f || (!td.child_count && pd.label.empty())) {
      labels_changed |= td.setLabelVisibility(false);
      continue;
    }
    auto pos = pd.position;
    bool parent_visible = true;				 
    for (int p = td.parent_node; p != -1; ) {
      auto & ptd = getNodeTertiaryData(p);
      if (p != active_child_node) {
	pos = getNodeArray().getNodeData(p).position;
	parent_visible = false;
      } else {
	pos += getNodeArray().getNodeData(p).position;
      }
      p = ptd.parent_node;
    }

    if (!display.isPointVisible(pos)) {
      labels_changed |= td.setLabelVisibility(false);
      continue;
    }
    float size = size_method.calculateSize(td, total_indegree, total_outdegree, nodes->size());
    if (td.child_count) {
      bool is_open = false;
      float score = 0.0f;
      if (td.child_count >= 2) {
	auto ppos = display.project(pos);
	auto d = ppos - display.project(pos + glm::vec3(size, 0.0f, 0.0f));
	auto d2 = ppos - glm::vec3(display.getViewport()[0] / 2.0f, display.getViewport()[1], 0.0f);
	float l = glm::length(d);
	is_open = l >= 100.0f;
	score = glm::length(d2);
	if (l >= 10.0f) {
	  labels_changed |= td.setLabelVisibility(true);	    
	} else {
	  labels_changed |= td.setLabelVisibility(false);
	}
      } else {
	labels_changed |= td.setLabelVisibility(false);
      }
      if (is_open && (best_child == -1 || score > best_score)) {
	best_child = *it;
	best_score = score;
      } else {
	// structure_changed |= td.toggleNode(false);	
      }
    } else if (pd.type == NODE_URL || pd.type == NODE_HASHTAG) {
      labels_changed |= td.setLabelVisibility(true);
    } else {
      float priority = 1000;
      if (has_priority_column) {
	priority = node_priority_column.getDouble(*it);
      }
      all_labels.push_back({ label_data_s::NODE, pos, glm::vec2(), size, priority, *it });
    }
  }

  if (setActiveChildNode(best_child)) {
    if (best_child != -1) {
      auto & td = node_geometry3[best_child];
      if (!td.isInitialized()) {
	randomizeChildGeometry(best_child, true);
      }
    }
    nodes->resume();
    structure_changed = true;
  }

  for (int i = 0; i < getFaceCount(); i++) {
    auto & fd = getFaceAttributes(i);
    if (!display.isPointVisible(fd.centroid)) {
      labels_changed |= fd.setLabelVisibility(false);
    } else if (!fd.label.empty() || getDefaultSymbolId()) {
      glm::vec3 pos(fd.centroid.x, fd.centroid.y, 0.0f);
      float priority = 1000;
      if (has_priority_column) {
	priority = face_priority_column.getDouble(i);
      }
      all_labels.push_back({ label_data_s::FACE, pos, glm::vec2(), 1.0f, priority, i });
    }
  }
    
  sort(all_labels.begin(), all_labels.end(), comparePriority);
  
  vector<label_data_s> drawn_labels;
  // const Rect2d & region = getContentRegion();
  
  for (auto & ld : all_labels) {
    auto tmp = display.project(ld.world_pos);
    ld.screen_pos = glm::vec2(tmp.x, tmp.y);
    auto & my_pos = ld.screen_pos;
    
    bool fits = true;
    
    for (auto & ld2 : drawn_labels) {
      auto & other_pos = ld2.screen_pos;
      
      float dx = fabsf(my_pos.x - other_pos.x);
      float dy = fabsf(my_pos.y - other_pos.y);
      
      if (dx < 200 && dy < 100) {
	fits = false;
      }
    }
    if (fits) {
      drawn_labels.push_back(ld);
    }
    if (ld.type == label_data_s::NODE) {
      labels_changed |= updateNodeLabelValues(ld.index, fits ? 1.00f : -1.00f);
    } else {
      labels_changed |= updateFaceLabelValues(ld.index, fits ? 1.00f : -1.00f);
    }
  }
  
  if (labels_changed) incLabelVersion();
  if (structure_changed) {
    cerr << "graph structure changed in updateVisibilities()" << endl;
    assert(nodes->isDynamic());
    incVersion();
  }

  return labels_changed;
}

Graph *
Graph::getGraphById2(int graph_id) {
  if (graph_id == getId()) {
    return this;
  } else {
    for (auto & gd : nested_graphs) {
      Graph * r = gd.second->getGraphById2(graph_id);
      if (r) return r;
    }
  }
  return 0;
}

const Graph *
Graph::getGraphById2(int graph_id) const {
  if (graph_id == getId()) {
    return this;
  } else {
    for (auto & gd : nested_graphs) {
      const Graph * r = gd.second->getGraphById2(graph_id);
      if (r) return r;
    }
  }
  return 0;
}

GraphRefR
Graph::getGraphForReading(int graph_id, const char * debug_name) const {
  double t0 = DateTime::getCurrentTime();
  const Graph * ptr = getGraphById2(graph_id);
  auto graph = GraphRefR(ptr);
  double t = DateTime::getCurrentTime() - t0;
  if (t > 0.01) {
    cerr << "getGraphForReading() took too long (" << t << ")";
    if (debug_name) cerr << " for " << debug_name << endl;
    else cerr << endl;
  }
  return graph;
}

GraphRefW
Graph::getGraphForWriting(int graph_id, const char * debug_name) {
  double t0 = DateTime::getCurrentTime();
  Graph * ptr = getGraphById2(graph_id);
  auto graph = GraphRefW(ptr); 
  double t = DateTime::getCurrentTime() - t0;
  if (t > 0.01) {
    cerr << "getGraphForWriting() took too long (" << t << ")";
    if (debug_name) cerr << " for " << debug_name << endl;
    else cerr << endl;
  }
  return graph;
}

glm::vec3
Graph::getNodePosition(int node_id) const {
  auto & td = getNodeTertiaryData(node_id);
  glm::vec3 pos = nodes->getNodeData(node_id).position;
  for (int p = td.parent_node; p != -1; p = getNodeTertiaryData(p).parent_node) {
    pos += nodes->getNodeData(p).position;
  }
  return pos;
}
  
skey
Graph::getNodeKey(int node_id) const {
  const table::Column & source_id_column = getNodeArray().getTable()["source"];
  const table::Column & id_column = getNodeArray().getTable()["id"];
  
  short source_id = source_id_column.getInt(node_id);
  long long source_object_id = 0;

  if (source_id) {
    source_object_id = id_column.getInt64(node_id);
  } else {
    source_object_id = node_id;
  }
  
  skey key(source_id, source_object_id);
  return key;
}

void
Graph::invalidateVisibleNodes() {
  if (final_graph.get()) final_graph->removeAllChildren();
  final_graph.reset();
  if (getFilter().get()) getFilter()->reset();
  for (auto & gd : nested_graphs) {
    gd.second->invalidateVisibleNodes();      
  }
}

void
Graph::setNodeTexture(const skey & key, int texture) { 
  auto it2 = getNodeArray().getNodeCache().find(key);
  if (it2 != getNodeArray().getNodeCache().end()) {
    getNodeArray().setNodeTexture(it2->second, texture);
  }
  if (final_graph.get()) final_graph->setNodeTexture(key, texture);
}

void
Graph::updateFaceAppearance() {
  auto & label_method = nodes->getLabelMethod();
  if (label_method.getValue() != LabelMethod::FIXED_LABEL) {
    for (int i = 0; i < getFaceCount(); i++) {
      string label, name, text, id;
      if (label_method.getValue() == LabelMethod::LABEL_FROM_COLUMN) {
	label = getFaceData()[label_method.getColumn()].getText(i);
      } else {
	for (auto & cd : getFaceData().getColumns()) {
	  string n = StringUtils::toLower(cd.first);
	  if (n == "label") {
	    label = cd.second->getText(i);
	  } else if (n == "name") {
	    name = cd.second->getText(i);
	  } else if (n == "text") {
	    text = cd.second->getText(i);
	  } else if (n == "id") {
	    id = cd.second->getText(i);
	  }
	}
	if (label_method.getValue() == LabelMethod::AUTOMATIC_LABEL && !label.empty()) {
	  if (!name.empty()) {
	    label = name;
	  } else if (!text.empty()) {
	    label = text;
	  } else if (!id.empty()) {
	    label = id;
	  }    
	}
      }
      // if (!label.empty()) cerr << "setting label for face " << i << ": " << label << endl;
      getFaceAttributes(i).setLabel(label);
    }
  }
}

void
Graph::applyGravity(float gravity, std::vector<node_position_data_s> & v) const {
  float k = nodes->getAlpha() * gravity;
  if (k < EPSILON) return;
  
  auto end = end_visible_nodes();
  for (auto it = begin_visible_nodes(); it != end; ++it) {
    auto & td = getNodeTertiaryData(*it);
    if (!td.isFixed()) {
      bool open = td.parent_node == active_child_node;

      if (open) {
	auto & pd = v[*it];
	float factor = 1.0f;
#if 1
	if (td.parent_node >= 0) {
	  factor = 28.0f;
	}
#endif
	float weight = nodes->hasTemporalCoverage() ? td.coverage_weight : 1.0f;
	const glm::vec3 & pos = pd.position;
	float d = glm::length(pos);
	if (d > 0.001) {
	  pd.position -= pos * (factor * k * sqrtf(d) / d * weight);
	}
      }
    }
  }
}

void
Graph::applyDrag(RenderMode mode, float friction, std::vector<node_position_data_s> & v) const {
  auto end = end_visible_nodes();
  for (auto it = begin_visible_nodes(); it != end; ++it) {
    auto & pd = v[*it];
    glm::vec3 & pos = pd.position, & ppos = pd.prev_position;
    glm::vec3 new_pos = pos - (ppos - pos) * friction;
    if (mode == RENDERMODE_2D) {
      new_pos.z = 0;
    }
    pd.prev_position = pos;
    pd.position = new_pos;
  }
}

void
Graph::applyAge() {
  auto end = end_visible_nodes();
  for (auto it = begin_visible_nodes(); it != end; ++it) {
    if (node_geometry3.size() <= *it) node_geometry3.resize(*it + 1);
    auto & td = node_geometry3[*it];
    td.age += 1.0f / 50.0f;
  }
  assert(nodes->isDynamic());
  incVersion();
}

bool
Graph::applyFilter(time_t start_time, time_t end_time, float start_sentiment, float end_sentiment) {
  if (getFilter().get() && final_graph.get()) {
    return getFilter()->apply(*final_graph, start_time, end_time, start_sentiment, end_sentiment, *this, statistics);
  } else {
    assert(0);
    return false;
  }
}

void
Graph::reset() {
  if (getFilter().get()) getFilter()->reset();  
}

bool
Graph::isNodeVisible(int node) const {
  if (node >= node_geometry3.size()) {
    return false;
  } else {
    // PROBLEM: node might not be visible even with children, since the children might be invisible
    return node_geometry3[node].first_edge != -1 || node_geometry3[node].child_count > 0 || node_geometry3[node].indegree > 0;
  }
}

int
Graph::addEdge(int n1, int n2, int face, float weight, int arc, long long coverage) {
  assert(n1 != -1 && n2 != -1);
  assert(weight >= 0);
  int edge = (int)edge_attributes.size();

  if (!isNodeVisible(n1)) {
    nodes->updateNodeAppearance(n1);
    setNodeAge(n1, initial_node_age); // this is first edge
  }
  int next_node_edge = getNodeFirstEdge(n1);
  
  setNodeFirstEdge(n1, edge);

  if (n1 != n2) {
    if (!isNodeVisible(n2)) {
      nodes->updateNodeAppearance(n2);
      setNodeAge(n2, initial_node_age); // this is first edge
    }
    updateOutdegree(n1, weight);
    updateIndegree(n2, weight);
  }
  updateNodeCoverage(n1, coverage);
  updateNodeCoverage(n2, coverage);
  
  edge_attributes.push_back(edge_data_s( weight, n1, n2, next_node_edge, -1, -1, arc, coverage ));
  total_edge_weight += fabsf(weight);
  if (weight > max_edge_weight) max_edge_weight = weight;

  if (face != -1) {
    setEdgeFace(edge, face);
  }
  
  incVersion();
  return edge;
}

void
Graph::addChild(int parent, int child) {
  assert(parent >= 0 && parent < nodes->size());
  assert(child >= 0 && child < nodes->size());
  assert(parent != child);
  
  if (node_geometry3.size() <= parent) node_geometry3.resize(parent + 1);
  if (node_geometry3.size() <= child) node_geometry3.resize(child + 1);

#if 0
  if (!isNodeVisible(parent)) {
    // PROBLEM: parent doesn't actually become visible, if the added child has no edges or children with edges
    nodes->updateNodeAppearance(parent);
    setNodeAge(parent, initial_node_age);
  }
#endif
  
  assert(node_geometry3[child].parent_node == -1);
  assert(node_geometry3[child].next_child == -1);
  
  node_geometry3[child].next_child = node_geometry3[parent].first_child;  
  node_geometry3[parent].first_child = child;
  node_geometry3[child].parent_node = parent;
  node_geometry3[parent].child_count++;
  nodes->getNodeData(parent).label_texture = 0;
  node_geometry3[parent].coverage = 0xffffffffffffffffULL;
  node_geometry3[parent].coverage_weight = 1.0f;
  nodes->getNodeData(child).position -= nodes->getNodeData(parent).position;

  assert(nodes->isDynamic());
  incVersion();
  
  assert(node_geometry3[child].parent_node == parent);
}

int
Graph::removeChild(int child) {
  if (node_geometry3.size() <= child) node_geometry3.resize(child + 1);
  int parent = node_geometry3[child].parent_node;
  assert(parent != -1);
  if (parent != -1) {
    if (node_geometry3[parent].first_child == child) {
      node_geometry3[parent].first_child = node_geometry3[child].next_child;
    } else {
      int n = node_geometry3[parent].first_child;
      while (n != -1) {
	int next_child = node_geometry3[n].next_child;
	if (next_child == child) {
	  node_geometry3[n].next_child = node_geometry3[child].next_child;
	  break;
	}
	n = next_child;
	assert(n != -1);
      }
    }
    node_geometry3[child].parent_node = node_geometry3[child].next_child = -1;
    node_geometry3[parent].child_count--;
    nodes->getNodeData(parent).label_texture = 0;
    nodes->getNodeData(child).position += nodes->getNodeData(parent).position;

    assert(nodes->isDynamic());
    incVersion();
  }
  return parent;
}

// insert the node in comm with which it shares dnodecomm links
void
Graph::addChild(int parent, int child, float dnodecomm) {
  addChild(parent, child);
  
  if (node_geometry3.size() <= parent) node_geometry3.resize(parent + 1);

  assert(parent >= 0 && parent < nodes->size());
  
  auto & td = node_geometry3[parent];

  td.louvain_tot += weightedDegree(child);
  td.louvain_in += 2*dnodecomm + numberOfSelfLoops(child);
}

// remove the node from its current community with which it has dnodecomm links
int
Graph::removeChild(int child, float dnodecomm) {
  int parent = removeChild(child);

  if (node_geometry3.size() <= parent) node_geometry3.resize(parent + 1);
  auto & td = node_geometry3[parent];
  td.louvain_tot -= weightedDegree(child);
  td.louvain_in -= 2*dnodecomm + numberOfSelfLoops(child);

  return parent;
}

void
Graph::removeAllChildren() {
  assert(nodes->isDynamic());
  for (int i = 0; i < nodes->size(); i++) {
    if (i < node_geometry3.size()) {
      auto & nd = nodes->getNodeData(i);
      auto & td = node_geometry3[i];
      if (td.parent_node != -1) {
	nd.position = getNodePosition(i);
	td.parent_node = -1;
      }
      td.setLabelVisibility(false);
    }
  }
  for (int i = 0; i < nodes->size(); i++) {
    if (i < node_geometry3.size()) {
      auto & nd = nodes->getNodeData(i);
      auto & td = node_geometry3[i];
      td.child_count = 0;
      td.first_child = -1;
      td.next_child = -1;
      td.louvain_tot = 0.0;
      td.louvain_in = 0.0;
      td.group_leader = -1;
      td.setIsInitialized(false);
    }
  }
}

std::unordered_map<int, float>
Graph::getAllNeighbors(int node) const {
  std::unordered_map<int, float> r;

  auto end = end_edges();
  for (auto it = begin_edges(); it != end; ++it) {
    int head = it->head, tail = it->tail;
    assert(tail < node_geometry3.size());
    assert(head < node_geometry3.size());
    while (head != -1 && tail != -1) {
      if (tail == node && head != node) {
	r[head] += it->weight;
	break;
      } else if (head == node && tail != node) {
	r[tail] += it->weight;
	break;
      }
      tail = node_geometry3[tail].parent_node;
      head = node_geometry3[head].parent_node;
    }
  }

  return r;
}
 
double
Graph::modularity() const {
  double q = 0.0;
  double total_weight = getTotalWeightedIndegree();
  assert(total_weight >= 0);

  size_t size = getNodeArray().size();
  for (int i = 0; i < size; i++) {
    auto & td = getNodeTertiaryData(i);
    if (td.louvain_tot > 0) {
      q += td.louvain_in / total_weight - (td.louvain_tot / total_weight) * (td.louvain_tot / total_weight);
    }
  }
  
  return q;
}

double
Graph::modularityGain(int node, int comm, double dnodecomm, double w_degree) const {
  assert(node >= 0 && node < getNodeArray().size());
  auto & td_comm = getNodeTertiaryData(comm);
  
  double totc = td_comm.louvain_tot;
  double degc = w_degree;
  double m2 = getTotalWeightedIndegree();
  double dnc = dnodecomm;
  
  return (dnc - totc * degc / m2);
}


void
NodeArray::resume() {
  if (active_child_node == -1) {
    getNodeArray().setTopLevelAlpha(INITIAL_ALPHA);
  } else {
    getNodeArray().getNodeData(active_child_node).alpha = INITIAL_ALPHA;
  }
}

void
NodeArray::updateAlpha() {
  if (active_child_node == -1) {
    getNodeArray().updateTopLevelAlpha();
  } else {
    getNodeArray().getNodeData(active_child_node).alpha *= 0.990f;
  }
}
