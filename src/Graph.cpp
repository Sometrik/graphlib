#include "Graph.h"

#include "DisplayInfo.h"
#include "ColorProvider.h"
#include "Controller.h"
#include "RenderMode.h"
#include "Label.h"

#include <algorithm>
#include <iostream>
#include <typeinfo>
#include <unordered_set>

#include <sys/time.h>

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

  std::unordered_set<int> open_nodes;
  open_nodes.insert(-1);
  for (int p = getActiveChildNode(); p != -1; p = getNodeTertiaryData(p).parent_node) {
    open_nodes.insert(p);
  }

  vector<Label> primary_labels;
  
  auto nodes_end = end_visible_nodes();
  for (auto it = begin_visible_nodes(); it != nodes_end; ++it) {
    auto & pd = getNodeArray().getNodeData(*it);
    auto & td = getNodeTertiaryData(*it);
    if (!(td.isLabelVisible() && pd.label_texture)) continue;

    float scale = 1.0f;
    auto pos = pd.position;
    for (int p = td.parent_node; p != -1; p = getNodeTertiaryData(p).parent_node, scale *= 0.125f) {
      if (!open_nodes.count(p)) {
	scale = 1.0f;
	pos = glm::vec3();
      }
      pos *= 0.125f;
      pos += getNodeArray().getNodeData(p).position;
    }
    
    glm::vec2 offset;
    unsigned short flags = 0;

    glm::vec4 color1 = black, color2 = white;
    if (td.hasChildren()) {
      float size = size_method.calculateSize(td, total_indegree, total_outdegree, nodes->size());
      color1 = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f);
      // offset += glm::vec2(0, 0.5 * size);
      pos += glm::vec3(0.0, size * scale, 0.0);
      flags |= LABEL_FLAG_CENTER;
      flags |= LABEL_FLAG_MIDDLE;

      labels.push_back({ pos, offset, pd.label_texture, flags, color1, color2 });
    } else if (pd.type == NODE_HASHTAG) {
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

// Gauss-Seidel relaxation for links
void
Graph::relaxLinks(std::vector<node_position_data_s> & v) const {
  // unsigned int visible_nodes = calcVisibleNodeCount();
  // double avg_edge_weight = total_edge_weight / getEdgeCount();
  float alpha = getAlpha();
  auto & size_method = nodes->getNodeSizeMethod();
  bool flatten = nodes->doFlattenHierarchy();
  unsigned int num_nodes = nodes->size();
  // float max_idf = log(visible_nodes / 1.0f);
  vector<bool> processed_edges;  
  processed_edges.resize(num_nodes * num_nodes);			
  auto end = end_edges();
  for (auto it = begin_edges(); it != end; ++it) {
    if (it->weight < 0.2f) continue;
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
      unsigned int key1 = tail * num_nodes + head;
      assert(key1 < processed_edges.size());
      if (processed_edges[key1]) continue;
      unsigned int key2 = head * num_nodes + tail;
      processed_edges[key1] = processed_edges[key2] = true;
      level = l1;
    }
    if (tail == head || (it->weight > -EPSILON && it->weight < EPSILON)) continue;
    
    auto & td1 = getNodeTertiaryData(tail), & td2 = getNodeTertiaryData(head);

    if (getNodeTertiaryData(tail).parent_node != active_child_node) {
      continue;
    }
    if (getNodeTertiaryData(head).parent_node != active_child_node) {
      continue;
    }
    
    auto & pd1 = v[tail], & pd2 = v[head];
    glm::vec3 & pos1 = pd1.position, & pos2 = pd2.position;
    glm::vec3 d = pos2 - pos1;
    float l = glm::length(d);

    if (l < EPSILON) continue;

    d *= 1 / l;
      
    float w1 = size_method.calculateSize(td1, total_indegree, total_outdegree, nodes->size());
    float w2 = size_method.calculateSize(td2, total_indegree, total_outdegree, nodes->size());

    if (td1.hasChildren()) l -= w1;
    if (td2.hasChildren()) l -= w2;

    if (l < EPSILON) continue;

    assert(td1.parent_node == td2.parent_node);

    float degree1 = td1.outdegree, degree2 = td2.indegree;
    float degree = degree1 > degree2 ? degree1 : degree2;
    if (degree == 0) degree = 1;
    float idf = 1.0f; // log(visible_nodes / degree) / max_idf;
    // float a = idf > 1.0 ? 1.0 : idf;
    
    // d *= getAlpha() * it->weight * link_strength * (l - link_length) / l;
    // d *= alpha * fabsf(it->weight) / max_edge_weight; // / avg_edge_weight;
    // l *= (level == 0 ? alpha : alpha / 48.0f) * idf;
    l *= alpha * idf * it->weight;
    
    float k = w1 / (w1 + w2);
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
  
  std::unordered_set<int> open_nodes;
  open_nodes.insert(-1);
  for (int p = getActiveChildNode(); p != -1; p = getNodeTertiaryData(p).parent_node) {
    open_nodes.insert(p);
  }

  auto end = end_visible_nodes();
  for (auto it = begin_visible_nodes(); it != end; ++it) {
    auto & pd = nodes->getNodeData(*it);
    auto & td = getNodeTertiaryData(*it);

    if (pd.type == NODE_HASHTAG || pd.type == NODE_ATTRIBUTE || pd.type == NODE_COMMUNITY) continue;
	
    float scale = 1.0f;
    auto pos = pd.position;
    for (int p = td.parent_node; p != -1; p = getNodeTertiaryData(p).parent_node, scale *= 0.125f) {
      if (!open_nodes.count(p)) {
	scale = 1.0f;
	pos = glm::vec3();
      }
      pos *= 0.125f;
      pos += getNodeArray().getNodeData(p).position;
    }

    float size = size_method.calculateSize(td, total_indegree, total_outdegree, nodes->size()) * scale;
    
    glm::vec3 tmp1 = display.project(pos);
    glm::vec3 tmp2 = display.project(pos + glm::vec3(size / 2.0f / node_scale, 0.0f, 0.0f));
    glm::vec2 pos1(tmp1.x, tmp1.y);
    glm::vec2 pos2(tmp2.x, tmp2.y);
    glm::vec2 tmp3 = pos2 - pos1;
    float diam = glm::length(tmp3);
    pos1 -= ppos;
    float d = glm::length(pos1) - diam;
    if (best_i == -1 || d < best_d) { // d <= 0
      best_i = *it;
      best_d = d;
    }
  }
  if (best_i != -1) {
    cerr << "best node " << best_i << ", d = " << best_d << ": " << getNodeArray().getNodeLabel(best_i) << endl;
  }
  return best_i;
}

void
Graph::refreshLayouts() {
  cerr << "resume after refreshLayouts\n";
  resume();
  if (final_graph.get()) final_graph->resume();
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
    
    statistics.clear();
  }
    
  statistics.setSentimentRange(start_sentiment, end_sentiment);
  
  bool changed = false;
  assert(final_graph.get());
  cerr << "trying to apply filter: st = " << int(start_time) << ", et = " << int(end_time) << ", ss = " << start_sentiment << ", es = " << end_sentiment << "\n";
  if (applyFilter(start_time, end_time, start_sentiment, end_sentiment)) {
    cerr << "filter changed the graph\n";
    final_graph->incVersion();
    assert(nodes->isDynamic());
    incVersion();
    resume();
    final_graph->resume();
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
  double total_edge_weight = 0;
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
  resume();
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
  bool labels_changed = false;

  bool has_priority_column = !label_method.getPriorityColumn().empty();
  auto & node_priority_column = nodes->getTable()[label_method.getPriorityColumn()];
  auto & face_priority_column = getFaceData()[label_method.getPriorityColumn()];

  int best_child = -1;
  float best_score = 0.0f;

  if (node_geometry3.size() < getNodeArray().size()) node_geometry3.resize(getNodeArray().size());

  std::unordered_set<int> open_nodes;
  open_nodes.insert(-1);
  for (int p = getActiveChildNode(); p != -1; p = getNodeTertiaryData(p).parent_node) {
    open_nodes.insert(p);
  }

  auto end = end_visible_nodes();
  for (auto it = begin_visible_nodes(); it != end; ++it) {
    auto & pd = getNodeArray().getNodeData(*it);
    auto & td = node_geometry3[*it];
    if (!td.hasChildren()) {
      continue;
    }
    float scale = 1.0f;
    auto pos = pd.position;
    for (int p = td.parent_node; p != -1; p = getNodeTertiaryData(p).parent_node) {
      if (!open_nodes.count(p)) {
	scale = 1.0f;
	pos = glm::vec3();
      }
      pos *= 0.125f;
      scale *= 0.125f;
      pos += getNodeArray().getNodeData(p).position;    
    }
    float size = size_method.calculateSize(td, total_indegree, total_outdegree, nodes->size()) * scale;

    auto ppos = display.project(pos);
    auto d = ppos - display.project(pos + glm::vec3(size, 0.0f, 0.0f));
    auto d2 = ppos - glm::vec3(display.getViewport()[2] / 2.0f, display.getViewport()[3], 0.0f);
    float l = glm::length(d);
    bool is_open = l >= 100.0f;
    float score = glm::length(d2);
    if (l >= 10.0f) {
      labels_changed |= td.setLabelVisibility(true);	    
    } else {
      labels_changed |= td.setLabelVisibility(false);
    }
    if (is_open && (best_child == -1 || score < best_score)) {
      best_child = *it;
      best_score = score;
    }
  }

  if (setActiveChildNode(best_child)) {
    if (best_child != -1) {
      auto & td = node_geometry3[best_child];
      if (!td.isInitialized()) {
	randomizeChildGeometry(best_child, true);
	resume();
	td.setIsInitialized(true);
      }
    }
    incVersion();
  }

  for (auto it = begin_visible_nodes(); it != end; ++it) {
    auto & pd = getNodeArray().getNodeData(*it);
    auto & td = node_geometry3[*it];
    if (pd.type == NODE_ATTRIBUTE || pd.type == NODE_IMAGE) {
      continue;
    } else if (td.hasChildren()) {
      continue;
    }
    float scale = 1.0f;
    auto pos = pd.position;
    for (int p = td.parent_node; p != -1; p = getNodeTertiaryData(p).parent_node) {
      if (!open_nodes.count(p)) {
	scale = 1.0f;
	pos = glm::vec3();
      }
      pos *= 0.125f;
      pos += getNodeArray().getNodeData(p).position;    
    }

    if (!display.isPointVisible(pos)) {
      labels_changed |= td.setLabelVisibility(false);
      continue;
    }
    float size = size_method.calculateSize(td, total_indegree, total_outdegree, nodes->size());
    if (pd.type == NODE_HASHTAG) {
      labels_changed |= td.setLabelVisibility(true);
    } else {
      float priority = 1000;
      if (has_priority_column) {
	priority = node_priority_column.getDouble(*it);
      }
      all_labels.push_back({ label_data_s::NODE, pos, glm::vec2(), size, priority, *it });
    }
  }

  for (int i = 0; i < getFaceCount(); i++) {
    auto & fd = getFaceAttributes(i);
    if (!display.isPointVisible(fd.centroid)) {
      labels_changed |= fd.setLabelVisibility(false);
    } else {
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

  return labels_changed;
}

static double getCurrentTime() {
  struct timeval tv;
  struct timezone tz;
  int r = gettimeofday(&tv, &tz);
  double t = 0;
  if (r == 0) {
    t = (double)tv.tv_sec + tv.tv_usec / 1000000.0;
  }
  return t;
}

GraphRefR
Graph::lockGraphForReading(const char * debug_name) const {
  double t0 = getCurrentTime();
  auto graph = GraphRefR(this);
  double t = getCurrentTime() - t0;
  if (t > 0.01) {
    cerr << "lockGraphForReading() took too long (" << t << ")";
    if (debug_name) cerr << " for " << debug_name << endl;
    else cerr << endl;
  }
  return graph;
}

GraphRefW
Graph::lockGraphForWriting(const char * debug_name) {
  double t0 = getCurrentTime();
  auto graph = GraphRefW(this);
  double t = getCurrentTime() - t0;
  if (t > 0.01) {
    cerr << "lockGraphForWriting() took too long (" << t << ")";
    if (debug_name) cerr << " for " << debug_name << endl;
    else cerr << endl;
  }
  return graph;
}

glm::vec3
Graph::getNodePosition(int node_id) const {
  std::unordered_set<int> open_nodes;
  open_nodes.insert(-1);
  for (int p = getActiveChildNode(); p != -1; p = getNodeTertiaryData(p).parent_node) {
    open_nodes.insert(p);
  }

  auto & td = getNodeTertiaryData(node_id);
  glm::vec3 pos = nodes->getNodeData(node_id).position;
  for (int p = td.parent_node; p != -1; p = getNodeTertiaryData(p).parent_node) {
    if (!open_nodes.count(p)) {
      pos = glm::vec3();
    }
    pos *= 0.125f;
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
}

void
Graph::setNodeTexture(const skey & key, int texture) { 
  auto it2 = getNodeArray().getNodeCache().find(key);
  if (it2 != getNodeArray().getNodeCache().end()) {
    getNodeArray().setNodeTexture(it2->second, texture);
  }
  if (final_graph.get()) final_graph->setNodeTexture(key, texture);
}

std::string
Graph::getFaceLabel(int face_id) const {
  string label, name, text, id;
  auto & label_method = nodes->getLabelMethod();
  if (label_method.getValue() == LabelMethod::LABEL_FROM_COLUMN) {
    label = getFaceData()[label_method.getColumn()].getText(face_id);
  } else {
    for (auto & cd : getFaceData().getColumns()) {
      auto & n = cd.first;
      if (strcasecmp(n.c_str(), "label") == 0) {
	label = cd.second->getText(face_id);
      } else if (strcasecmp(n.c_str(), "name") == 0) {
	name = cd.second->getText(face_id);
      } else if (strcasecmp(n.c_str(), "text") == 0) {
	text = cd.second->getText(face_id);
      } else if (strcasecmp(n.c_str(), "id") == 0) {
	id = cd.second->getText(face_id);
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
  return label;
}

void
Graph::applyGravity(float gravity, std::vector<node_position_data_s> & v) const {
  float k = getAlpha() * gravity;
  if (k < EPSILON) return;
  
  auto end = end_visible_nodes();
  for (auto it = begin_visible_nodes(); it != end; ++it) {
    auto & td = getNodeTertiaryData(*it);
    bool open = td.parent_node == active_child_node;
    
    if (open) {
      auto & pd = v[*it];
      float weight = 1.0f;
      // float weight = pow(td.size, 0.5f);
      const glm::vec3 & pos = pd.position;
      float d = glm::length(pos);
      if (d > 0.001) {
	// pd.position -= pos * (k * sqrtf(d) / d * weight);
	pd.position -= pos * k * weight;
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
    return node_geometry3[node].first_edge != -1 || node_geometry3[node].hasChildren() || node_geometry3[node].indegree > 0;
  }
}

int
Graph::addEdge(int n1, int n2, int face, float weight, int arc) {
  assert(n1 != -1 && n2 != -1);
  assert(weight >= 0);
  int edge = (int)edge_attributes.size();

  if (!isNodeVisible(n1)) {
    setNodeAge(n1, initial_node_age); // this is first edge
  }
  int next_node_edge = getNodeFirstEdge(n1);
  
  setNodeFirstEdge(n1, edge);
  updateOutdegree(n1, weight);
  updateIndegree(n2, weight);
  
  if (n1 == n2) {
    node_geometry3[n1].weighted_selfdegree += weight;
  } else if (!isNodeVisible(n2)) {
    setNodeAge(n2, initial_node_age); // this is first edge
  }
  
  edge_attributes.push_back(edge_data_s( weight, n1, n2, next_node_edge, -1, -1, arc ));
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
    setNodeAge(parent, initial_node_age);
  }
#endif
  
  assert(node_geometry3[child].parent_node == -1);
  assert(node_geometry3[child].next_child == -1);
  
  node_geometry3[child].next_child = node_geometry3[parent].first_child;  
  node_geometry3[parent].first_child = child;
  node_geometry3[child].parent_node = parent;
  node_geometry3[parent].child_count++;
  node_geometry3[parent].descendant_count += 1 + node_geometry3[child].descendant_count;
  nodes->getNodeData(parent).label_texture = 0;
  nodes->getNodeData(child).position = nodes->getNodeData(child).position / 0.125f - nodes->getNodeData(parent).position;

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
    node_geometry3[parent].descendant_count -= 1 + node_geometry3[child].descendant_count;
    nodes->getNodeData(parent).label_texture = 0;
    nodes->getNodeData(child).position = nodes->getNodeData(parent).position + nodes->getNodeData(child).position * 0.125f;

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

  td.weighted_indegree += node_geometry3[child].weighted_indegree;
  td.weighted_outdegree += node_geometry3[child].weighted_outdegree;
  td.weighted_selfdegree += dnodecomm + node_geometry3[child].weighted_selfdegree;
  td.indegree += node_geometry3[child].indegree;
  td.outdegree += node_geometry3[child].outdegree;  
}

// remove the node from its current community with which it has dnodecomm links
int
Graph::removeChild(int child, float dnodecomm) {
  int parent = removeChild(child);

  if (node_geometry3.size() <= parent) node_geometry3.resize(parent + 1);
  auto & td = node_geometry3[parent];
  td.weighted_indegree -= node_geometry3[child].weighted_indegree;
  td.weighted_outdegree -= node_geometry3[child].weighted_outdegree;
  td.weighted_selfdegree -= dnodecomm + node_geometry3[child].weighted_selfdegree;
  td.indegree -= node_geometry3[child].indegree;
  td.outdegree -= node_geometry3[child].outdegree;  
  return parent;
}

void
Graph::flattenChildren(int new_parent, int old_parent) {
  if (old_parent == -1) old_parent = new_parent;
  
  if (node_geometry3.size() <= old_parent) node_geometry3.resize(old_parent + 1);
  for (int n = node_geometry3[old_parent].first_child; n != -1; n = node_geometry3[n].next_child) {
    flattenChildren(new_parent, n);
    if (node_geometry3[n].parent_node != new_parent) {
      removeChild(n);
      addChild(new_parent, n);
    }
  }
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
      if (td.hasChildren() || nd.type == NODE_COMMUNITY) {
	td.child_count = 0;
      	td.first_child = -1;
	td.indegree = td.outdegree = 0;
	td.weighted_indegree = 0.0f;
	td.weighted_outdegree = 0.0f;
	td.weighted_selfdegree = 0.0f;
      }
      td.descendant_count = 0;
      td.next_child = -1;
      td.group_leader = -1;
      td.setIsInitialized(false);
    }
  }
  active_child_node = -1;
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
  double m = getTotalWeightedIndegree() + getTotalWeightedOutdegree();
  assert(m >= 0);

  auto end = end_visible_nodes();
  for (auto it = begin_visible_nodes(); it != end; ++it) {
    auto & td = getNodeTertiaryData(*it);
    if (td.parent_node == -1) {
      if (getNodeArray().getNodeData(*it).type != NODE_COMMUNITY) {
	cerr << "got invalid node " << *it << ": type = " << int(getNodeArray().getNodeData(*it).type) << ", label = " << getNodeArray().getNodeLabel(*it) << endl;
      }
      if (td.weighted_indegree + td.weighted_outdegree > 0) {
	double tot_var = (td.weighted_indegree + td.weighted_outdegree) / m;
	q += 2 * td.weighted_selfdegree / m - tot_var * tot_var;
      }
    }
  }
  
  return q;
}

double
Graph::modularityGain(int node, int comm, double dnodecomm, double w_degree) const {
  assert(node >= 0 && node < getNodeArray().size());
  auto & td_comm = getNodeTertiaryData(comm);
  
  double totc = td_comm.weighted_indegree + td_comm.weighted_outdegree;
  double degc = w_degree;
  double m2 = getTotalWeightedIndegree() + getTotalWeightedOutdegree();
  double dnc = dnodecomm;
  
  return (dnc - totc * degc / m2);
}

double
Graph::directedModularity() const {
  double q = 0.0;
  double m = getTotalWeightedIndegree();
  assert(m >= 0);

  auto end = end_visible_nodes();
  for (auto it = begin_visible_nodes(); it != end; ++it) {
    auto & td = getNodeTertiaryData(*it);
    if (td.parent_node == -1 && (td.weighted_indegree > 0 || td.weighted_outdegree > 0)) {
      double tot_out_var = (double)td.weighted_outdegree / m;
      double tot_in_var = (double)td.weighted_indegree / m;
      q += td.weighted_selfdegree / m - (tot_out_var * tot_in_var);
    }
  }
  
  return q;
}

double
Graph::modularityGain(int node, int comm, double dnodecomm, double w_degree_out, double w_degree_in) const {
  assert(node >= 0 && node < getNodeArray().size());
  auto & td_comm = getNodeTertiaryData(comm);
  
  double totc_out = td_comm.weighted_outdegree;
  double totc_in = td_comm.weighted_indegree;
  double degc_out = w_degree_out;
  double degc_in = w_degree_in;
  double m2 = getTotalWeightedIndegree();
  double dnc = dnodecomm;
  
  return (dnc/m2 - ((degc_out*totc_in + degc_in*totc_out)/(m2*m2)));
}

void
Graph::resume() {
  if (active_child_node == -1) {
    getNodeArray().setTopLevelAlpha(INITIAL_ALPHA);
  } else {
    getNodeArray().getNodeData(active_child_node).alpha = INITIAL_ALPHA;
  }
}

void
Graph::updateAlpha() {
  if (active_child_node == -1) {
    getNodeArray().updateTopLevelAlpha();
  } else {
    getNodeArray().getNodeData(active_child_node).alpha *= 0.9925f;
  }
}
