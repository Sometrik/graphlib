#ifndef _GRAPH_H_
#define _GRAPH_H_

#include "RawStatistics.h"
#include "NodeArray.h"

#include <vector>
#include <set>
#include <unordered_map>
#include <string>
#include <vector>

#define FACE_LABEL_VISIBLE	1

#define NODE_IS_SELECTED	1
#define NODE_LABEL_VISIBLE	2
#define NODE_IS_INITIALIZED	4

struct node_tertiary_data_s {
  int first_edge = -1;
  int indegree = 0, outdegree = 0;
  float weighted_indegree = 0.0f, weighted_outdegree = 0.0f, weighted_selfdegree = 0.0f;
  int first_child = -1, next_child = -1, parent_node = -1;
  unsigned int child_count = 0, descendant_count = 0;
  unsigned short flags = NODE_IS_SELECTED;
  unsigned short label_visibility_val = 0;
  int group_leader = -1;

  bool hasChildren() const { return first_child != -1; }
  
  bool setGroupLeader(int id) {
    if (id != group_leader) {
      group_leader = id;
      return true;
    } else {
      return false;
    }
  }
  
  bool setIsInitialized(bool t) {
    if ((t && !isInitialized()) || (!t && isInitialized())) {
      if (t) flags |= NODE_IS_INITIALIZED;
      else flags &= ~NODE_IS_INITIALIZED;
      return true;
    } else {
      return false;
    }
  }

  bool isSelected() const { return flags & NODE_IS_SELECTED; }
  bool isLabelVisible() const { return flags & NODE_LABEL_VISIBLE; }
  bool isInitialized() const { return flags & NODE_IS_INITIALIZED; }

  bool setLabelVisibility(bool t) {
    if ((t && !isLabelVisible()) || (!t && isLabelVisible())) {
      if (t) flags |= NODE_LABEL_VISIBLE;
      else flags &= ~NODE_LABEL_VISIBLE;
      return true;
    } else {
      return false;
    }
  }

  float getLabelVisibilityValue() const { return label_visibility_val / 65535.0f; }

  void setLabelVisibilityValue(float f) {
    int f2 = int(f * 65535);
    if (f2 < 0) f2 = 0;
    else if (f2 > 65535) f2 = 65535;
    label_visibility_val = (unsigned short)f2;
  }
};

struct edge_data_s {
edge_data_s() : weight(1.0f), tail(-1), head(-1), next_node_edge(-1), face(-1), next_face_edge(-1), arc(0) { }
edge_data_s(float _weight, int _tail, int _head, int _next_node_edge, int _face, int _next_face_edge, int _arc)
: weight(_weight), tail(_tail), head(_head), next_node_edge(_next_node_edge), face(_face), next_face_edge(_next_face_edge), arc(_arc) { }
  
  float weight, weight_divisor = 1.0f;
  int tail, head, next_node_edge, face, next_face_edge, arc, pair_edge = -1, parent_edge = -1;
};

struct face_data_s {
  glm::vec2 centroid;
  int first_edge;
  time_t timestamp;
  float sentiment;
  short feed, lang;
  long long app_id;
  short label_texture, flags;
  unsigned short label_visibility_val;

  void setLabelVisibilityValue(float f) {
    int f2 = int(f * 65535);
    if (f2 < 0) f2 = 0;
    else if (f2 > 65535) f2 = 65535;
    label_visibility_val = (unsigned short)f2;
  }

  bool setLabelVisibility(bool t) {
    if ((t && !isLabelVisible()) || (!t && isLabelVisible())) {
      if (t) flags |= FACE_LABEL_VISIBLE;
      else flags &= ~FACE_LABEL_VISIBLE;
      return true;
    } else {
      return false;
    }
  }
  bool isLabelVisible() const { return flags & FACE_LABEL_VISIBLE; }
  float getLabelVisibilityValue() const { return label_visibility_val / 65535.0f; }
};

#include "EdgeIterator.h"
#include "VisibleNodeIterator.h"

class GraphRefR;
class GraphRefW;
class Label;
class GraphFilter;
class DisplayInfo;

class Graph {
 public:
  friend class GraphRefR;
  friend class GraphRefW;

  enum GraphStyle {
    DEFAULT_STYLE = 1,
    PACKED_SPHERES
  };
  
  Graph(int _id = 0)
    : id(_id),
    node_color(0.0f, 0.0f, 0.0f, 0.0f),
    edge_color(0.0f, 0.0f, 0.0f, 0.0f),
    face_color(0.0f, 0.0f, 0.0f, 0.0f)
      {
	if (!id) id = next_id++;
      }
  
  Graph(const Graph & other) = delete;
  Graph & operator= (const Graph & other) = delete;

  virtual ~Graph() { }
  
  table::Table & getFaceData() { return faces; }
  const table::Table & getFaceData() const { return faces; }

  bool empty() const { return getEdgeCount() == 0; }
  // double calculateTotalEnergy() const;

  void addChild(int parent, int child);
  void addChild(int parent, int child, float dnodecomm);
  int removeChild(int child);
  int removeChild(int child, float dnodecomm);
  void flattenChildren(int new_parent, int old_parent = -1);
  
  void removeAllChildren();

  bool isNodeVisible(int node) const;

  void setGroupLeader(int node, int leader) {
    if (node_geometry3.size() <= node) node_geometry3.resize(node + 1);
    node_geometry3[node].group_leader = leader;
  }

  void setIsInitialized(int node, bool t) {
    if (node_geometry3.size() <= node) node_geometry3.resize(node + 1);
    node_geometry3[node].setIsInitialized(t);
  }

  int getFaceFirstEdge(int i) const { return face_attributes[i].first_edge; }

  virtual bool isDirected() const { return true; }
  virtual std::shared_ptr<Graph> createSimilar() const;

  bool applyFilter(time_t start_time, time_t end_time, float start_sentiment, float end_sentiment);
  void reset();

  int getNextNodeEdge(int edge) const {
    return getEdgeAttributes(edge).next_node_edge;
  }
  int getEdgeTargetNode(int edge) const {
    return getEdgeAttributes(edge).head;
  }
  void setEdgeFace(int edge, int face) {
    auto & ed = getEdgeAttributes(edge);
    ed.face = face;
    if (face != -1) {
      ed.next_face_edge = getFaceFirstEdge(face);
      face_attributes[face].first_edge = edge;
    }
  }

  int addEdge(int n1, int n2, int face = -1, float weight = 1.0f, int arc_id = 0);

  void connectEdgePair(int e1, int e2) {
    getEdgeAttributes(e1).pair_edge = e2;
    getEdgeAttributes(e2).pair_edge = e1;
  }

  void setParentEdge(int child, int parent) {
    getEdgeAttributes(child).parent_edge = parent;
  }
  
  int getNextFaceEdge(int edge) const {
    return edge_attributes[edge].next_face_edge;
  }

  void setFaceLabelTexture(int i, int texture) {
    face_attributes[i].label_texture = texture;
    incLabelVersion();
  }

  bool updateFaceLabelValues(int i, float visibility) {
    auto & fd = face_attributes[i];
    float vv = fd.getLabelVisibilityValue() + visibility;
    if (vv < 0) vv = 0;
    else if (vv > 1) vv = 1;
    fd.setLabelVisibilityValue(vv);
    if (vv >= 0.75) return fd.setLabelVisibility(true);
    else if (vv <= 0.25) return fd.setLabelVisibility(false);
    else return false;
  }
  
  virtual void calculateRegionAreas() { }
  virtual void mapFacesToNodes(Graph & target) { }
  virtual void mapRegionsToNodes(Graph & target) { }
  virtual void colorizeRegions() { }
  virtual void spatialAggregation(const Graph & other) { }
  virtual void addUniversalRegion() { }
  
  void getVisibleLabels(std::vector<Label> & labels) const;

  virtual std::set<int> getAdjacentRegions() const { return std::set<int>(); }

  bool hasEdge(int n1, int n2) const;

  void setNodeFirstEdge(int n, int edge) {
    if (node_geometry3.size() <= n) node_geometry3.resize(n + 1);
    node_geometry3[n].first_edge = edge;
  }

  void updateOutdegree(int n, float weight) {
    if (node_geometry3.size() <= n) node_geometry3.resize(n + 1);
    node_geometry3[n].weighted_outdegree += weight;
    node_geometry3[n].outdegree++;
    total_outdegree++;
    total_weighted_outdegree += weight;
  }

  void updateIndegree(int n, float weight) {
    if (node_geometry3.size() <= n) node_geometry3.resize(n + 1);
    node_geometry3[n].weighted_indegree += weight;
    node_geometry3[n].indegree++;
    total_indegree++;
    total_weighted_indegree += weight;
  }
  
  // return the weighted degree of the node
  int out_degree(int node) const {
    int d = 0;
    int edge = getNodeFirstEdge(node);
    while (edge != -1) {
      d++;
      edge = getNextNodeEdge(edge);
    }
    return d;
  }
 
  int getNodeFirstEdge(int i) const {
    if (i >= 0 && i < node_geometry3.size()) {
      return node_geometry3[i].first_edge;
    } else {
      return -1;
    }
  }

  // return the weighted degree of the node
  float weightedDegree(int n) {
    if (node_geometry3.size() <= n) return 0.0f;
    return node_geometry3[n].weighted_indegree + node_geometry3[n].weighted_outdegree;    
  }

  unsigned int numberOfNeighbors(int n) {
    if (node_geometry3.size() <= n) return 0;
    return node_geometry3[n].indegree + node_geometry3[n].outdegree;
  }

  // float numberOfSelfLoops(int node);
  
  std::unordered_map<int, float> getAllNeighbors(int node) const;

  size_t getEdgeCount() const { return edge_attributes.size(); }
  size_t getFaceCount() const { return faces.size(); }  
  
  const glm::vec2 & getFaceCentroid(int i) const { return face_attributes[i].centroid; }
          
  void setKeywords(const std::string & k) { keywords = k; }
  const std::string & getKeywords() const { return keywords; }
  
  void randomizeGeometry(bool use_2d = false);
  void randomizeChildGeometry(int node_id, bool use_2d = false);
  void refreshLayouts();

  void setId(int _id) { id = _id; }
  int getId() const { return id; }

  void setName(const std::string & _name) { name = _name; }
  const std::string & getName() const { return name; }

  void setSourceId(short s) { source_id = s; }
  short getSourceId() const { return source_id; }

  void setServerSearchId(int id) { server_search_id = id; }
  int getServerSearchId() const { return server_search_id; }
  bool isLoaded() const { return is_loaded; }
  void setIsLoaded(bool t) { is_loaded = t; }
  
  unsigned int getNewPrimaryObjects() const { return new_primary_objects_counter; }
  unsigned int getNewSecondaryObjects() const { return new_secondary_objects_counter; }
  unsigned int getNewImages() const { return new_images_counter; }
  
  void updateNewPrimaryObjects(unsigned int i) { new_primary_objects_counter += i; }
  void updateNewSecondaryObjects(unsigned int i) { new_secondary_objects_counter += i; }
  void resetNewObjects2() { new_primary_objects_counter = new_secondary_objects_counter = new_images_counter = 0; }

  virtual int addFace(time_t timestamp = 0, float sentiment = 0, short feed = 0, short lang = 0, long long app_id = -1) { // int shell1 = -1, int shell2 = -1) {
    int face_id = (int)face_attributes.size();
    face_attributes.push_back({ glm::vec2(0, 0), -1, timestamp, sentiment, feed, lang, app_id, 0, 0 });
    faces.addRow();
    return face_id;
  }
  
  virtual bool checkConsistency() const { return true; }
          
  void setNodeTexture(const skey & key, int texture);
  
  void clearTextures(int clear_flags = CLEAR_ALL) {
    nodes->clearTextures(clear_flags);
    if (clear_flags & CLEAR_LABELS) {
      for (int i = 0; i < getFaceCount(); i++) {
	face_attributes[i].label_texture = 0;
      }
    }
  }

  void setNodeArray(const std::shared_ptr<NodeArray> & _nodes) { nodes = _nodes; }
  NodeArray & getNodeArray() { return *nodes; }
  const NodeArray & getNodeArray() const { return *nodes; }
      
  int getVersion() const { return version + nodes->getVersion(); }
  int getLabelVersion() const { return label_version + nodes->getVersion(); }

  void calculateEdgeCentrality();
      
  Graph & getActualGraph() {
    auto g = getFinal();
    return g.get() ? *g : *this;
  }
  const Graph & getActualGraph() const {
    auto g = getFinal();
    return g.get() ? *g : *this;
  }
  
  std::shared_ptr<Graph> getFinal() { return final_graph; }
  const std::shared_ptr<const Graph> getFinal() const { return final_graph; }
  
  void setFinalGraph(std::shared_ptr<Graph> g) { final_graph = g; }

  virtual void clear() {
    faces.clear();    
    face_attributes.clear();
    edge_attributes.clear();

    max_edge_weight = 0.0f;
    final_graph.reset();
    face_cache.clear();
    node_geometry3.clear();
    
    total_weighted_outdegree = total_weighted_indegree = 0.0;
    total_outdegree = total_indegree = 0;
    filter.reset();
    active_child_node = -1;
    manually_selected_active_child = false;
  }
      
  std::unordered_map<skey, int> & getFaceCache() { return face_cache; } 
  const std::unordered_map<skey, int> & getFaceCache() const { return face_cache; } 

  int getFaceId(short source_id, long long source_object_id) const;
  
  int pickNode(const DisplayInfo & display, int x, int y, float node_scale) const;
  
  face_data_s & getFaceAttributes(int i) { return face_attributes[i]; }
  const face_data_s & getFaceAttributes(int i) const { return face_attributes[i]; }

  edge_data_s & getEdgeAttributes(int i) { return edge_attributes[i]; }
  const edge_data_s & getEdgeAttributes(int i) const { return edge_attributes[i]; }
  EdgeIterator begin_edges() { return EdgeIterator(&(edge_attributes.front())); }
  EdgeIterator end_edges() {
    EdgeIterator it(&(edge_attributes.back()));
    ++it;
    return it;
  }
  ConstEdgeIterator begin_edges() const { return ConstEdgeIterator(&(edge_attributes.front())); }
  ConstEdgeIterator end_edges() const {
    ConstEdgeIterator it(&(edge_attributes.back()));
    ++it;
    return it;
  }

  ConstVisibleNodeIterator begin_visible_nodes() const { return ConstVisibleNodeIterator(&(edge_attributes.front()), &(edge_attributes.back()) + 1, &(node_geometry3.front()), &(node_geometry3.back()) + 1, nodes->size(), active_child_node); }
  ConstVisibleNodeIterator end_visible_nodes() const { return ConstVisibleNodeIterator(); }
  
  bool updateSelection(time_t start_time, time_t end_time, float start_sentiment, float end_sentiment);

  const RawStatistics & getStatistics() const { return statistics; }
  RawStatistics & getStatistics() { return statistics; }

  float getLineWidth() const { return line_width; }
  void setLineWidth(float w) { line_width = w; }

  bool updateVisibilities(const DisplayInfo & display, bool reset = false);

  bool updateNodeLabelValues(int n, float visibility) {
    if (node_geometry3.size() <= n) node_geometry3.resize(n + 1);
    auto & td = node_geometry3[n];
    float vv = td.getLabelVisibilityValue() + visibility;
    if (vv < 0) vv = 0;
    else if (vv > 1) vv = 1;
    td.setLabelVisibilityValue(vv);
    if (vv >= 0.75) return td.setLabelVisibility(true);
    else if (vv <= 0.25) return td.setLabelVisibility(false);
    else return false;
  }

  const node_tertiary_data_s & getNodeTertiaryData(int n) const {
    if (n >= 0 && n < node_geometry3.size()) {
      return node_geometry3[n];
    } else {
      return null_geometry3;
    }
  }

  int getNodeDepth(int n) const {
    int l = 0;
    for (int p = node_geometry3[n].parent_node; p != -1; p = node_geometry3[p].parent_node) l++;
    return l;
  }

  glm::vec3 getNodePosition(int node_id) const;
  
  skey getNodeKey(int node_id) const;
      
  void invalidateVisibleNodes();

  GraphRefR lockGraphForReading(const char * debug_name) const;
  GraphRefW lockGraphForWriting(const char * debug_name);
  
  std::string getFaceLabel(int face_id) const;
    
  void relaxLinks(std::vector<node_position_data_s> & v) const;

  double modularity() const; // calculate the modularity of the communities
  double directedModularity() const;
    
  double modularityGain(int node, int comm, double dnodecomm, double w_degree) const;
  double modularityGain(int node, int comm, double dnodecomm, double w_degree_out, double w_degree_in) const;
  
  double getTotalWeightedOutdegree() const { return total_weighted_outdegree; }
  double getTotalWeightedIndegree() const { return total_weighted_indegree; }
  unsigned int getTotalOutdegree() const { return total_outdegree; }
  unsigned int getTotalIndegree() const { return total_indegree; }

  void setDefaultSymbolId(int symbol_id) { default_symbol_id = symbol_id; }
  int getDefaultSymbolId() const { return default_symbol_id; }

  void setNodeVisibility(bool t) { show_nodes = t; }
  void setEdgeVisibility(bool t) { show_edges = t; }
  void setFaceVisibility(bool t) { show_faces = t; }
  void setLabelVisibility(bool t) { show_labels = t; }

  bool getNodeVisibility() const { return show_nodes; }
  bool getEdgeVisibility() const { return show_edges; }
  bool getFaceVisibility() const { return show_faces; }
  bool getLabelVisibility() const { return show_labels; }

  const glm::vec4 & getDefaultNodeColor() const { return node_color; }
  const glm::vec4 & getDefaultEdgeColor() const { return edge_color; }
  const glm::vec4 & getDefaultFaceColor() const { return face_color; }
  
  void setDefaultNodeColor(const glm::vec4 & color) { node_color = color; }
  void setDefaultEdgeColor(const glm::vec4 & color) { edge_color = color; }
  void setDefaultFaceColor(const glm::vec4 & color) { face_color = color; }

  void setVersion(int _version) { version = _version; }
  void incVersion() { version++; }

  void setFilter(const std::shared_ptr<GraphFilter> & _filter) { filter = _filter; }
  const std::shared_ptr<GraphFilter> & getFilter() const { return filter; }

  int getActiveChildNode() const { return active_child_node; }

  void updateAlpha();
  void resume();

  float getAlpha() const {
    if (active_child_node == -1) {
      return alpha;
    } else {
      return child_alpha;
    }
  }
  float isRunning() const { return getAlpha() >= 0.005f; }

  void selectNode(int node_id);

 protected:
  void incLabelVersion() { label_version++; }

  bool setActiveChildNode(int id);

  table::Table faces;
  std::vector<face_data_s> face_attributes;
  std::vector<edge_data_s> edge_attributes;
  float max_edge_weight = 0.0f;
  std::shared_ptr<NodeArray> nodes;
 
 private:
  void lockReader() const { nodes->lockReader(); }
  void unlockReader() const { nodes->unlockReader(); }
  void lockWriter() { nodes->lockWriter(); }
  void unlockWriter() { nodes->unlockWriter(); }
  
  int version = 1, label_version = 1;
  short source_id = 0;

  int id;
  unsigned int new_primary_objects_counter = 0, new_secondary_objects_counter = 0, new_images_counter = 0;
  std::shared_ptr<Graph> final_graph;
  std::unordered_map<skey, int> face_cache;
  RawStatistics statistics;
  std::string name, keywords;
  int server_search_id = 0;
  bool is_loaded = false;
  float line_width = 1.0f;
  std::vector<node_tertiary_data_s> node_geometry3;
  double total_weighted_outdegree = 0, total_weighted_indegree = 0;
  unsigned int total_outdegree = 0, total_indegree = 0;
  node_tertiary_data_s null_geometry3;
  int default_symbol_id = 0;
  bool show_nodes = true, show_edges = true, show_faces = true, show_labels = true;
  glm::vec4 node_color, edge_color, face_color;
  std::shared_ptr<GraphFilter> filter;
  int active_child_node = -1;
  bool manually_selected_active_child = false;
  float alpha = 0.0f;
  float child_alpha = 0.0f;
  
  static int next_id;
};

class GraphRefR {
public:
  GraphRefR(const Graph * _graph) : graph(_graph) {
    if (graph) graph->lockReader();
  }
  GraphRefR(const GraphRefR & other) : graph(other.graph) {
    if (graph) graph->lockReader();
  }
  ~GraphRefR() { if (graph) graph->unlockReader(); }
  GraphRefR & operator=(const GraphRefR & other) {
    if (&other != this) {
      if (graph) graph->unlockReader();
      graph = other.graph;
      if (graph) graph->lockReader();
    }
    return *this;
  }
  const Graph * operator->() const { return graph; }
  const Graph & operator*() const { return *graph; }
  const Graph * get() const { return graph; }

  void reset(Graph * ptr) {
    if (graph) graph->unlockReader();
    graph = ptr;
    if (graph) graph->lockReader();
  }
  
private:
  const Graph * graph;
};

class GraphRefW {
public:
  GraphRefW(Graph * _graph) : graph(_graph) {
    if (graph) graph->lockWriter();
  }
  GraphRefW(const GraphRefW & other) = delete;
  GraphRefW(GraphRefW && other) {
    graph = other.graph;
    other.graph = nullptr;
  }
  ~GraphRefW() { if (graph) graph->unlockWriter(); }
  GraphRefW & operator=(const GraphRefW & other) = delete;
  Graph * operator->() { return graph; }
  Graph & operator*() { return *graph; }
  const Graph * get() const { return graph; }
  Graph * get() { return graph; }

  void reset(Graph * ptr) {
    if (graph) graph->unlockWriter();
    graph = ptr;
    if (graph) graph->lockWriter();
  }
  
private:
  Graph * graph;
};

#endif
