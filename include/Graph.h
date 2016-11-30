#ifndef _GRAPH_H_
#define _GRAPH_H_

#include "GraphInterface.h"
#include "RawStatistics.h"
#include "NodeArray.h"
#include "graph_color.h"

#include <vector>
#include <set>
#include <unordered_map>
#include <string>
#include <vector>

#define FACE_LABEL_VISIBLE	1

#define NODE_SELECTED		1
#define NODE_LABEL_VISIBLE	2
#define NODE_FIXED_POSITION	4
#define NODE_IS_OPEN		8
#define NODE_IS_GROUP_LEADER	16

class Graph;
class DisplayInfo;

struct node_tertiary_data_s {
  int first_edge = -1;
  int indegree = 0, outdegree = 0;
  float weighted_indegree = 0.0f, weighted_outdegree = 0.0f, coverage_weight = 0.0f;
  long long coverage = 0;
  int first_child = -1, next_child = -1, parent_node = -1;
  unsigned int child_count = 0;
  float age = 0.0;
  unsigned short flags = NODE_SELECTED;
  unsigned short label_visibility_val = 0;
  float louvain_in = 0.0f, louvain_tot = 0.0f;
  
  void setNodeFixedPosition(int i, bool t) {
    if (t) flags |= NODE_FIXED_POSITION;
    else flags &= ~NODE_FIXED_POSITION;
    // doesn't affect anything directly, so no need to update version
  }
  
  bool toggleNode(bool t) {
    if ((t && !isGroupOpen()) || (!t && isGroupOpen())) {
      if (t) flags |= NODE_IS_OPEN;
      else flags &= ~NODE_IS_OPEN;
      return true;
    } else {
      return false;
    }
  }

  bool setIsGroupLeader(bool t) {
    if ((t && !isGroupLeader()) || (!t && isGroupLeader())) {
      if (t) flags |= NODE_IS_GROUP_LEADER;
      else flags &= ~NODE_IS_GROUP_LEADER;
      return true;
    } else {
      return false;
    }
  }

  bool isGroupOpen() const { return flags & NODE_IS_OPEN; }  
  bool isFixed() const { return flags & NODE_FIXED_POSITION; }
  bool isSelected() const { return flags & NODE_SELECTED; }
  bool isLabelVisible() const { return flags & NODE_LABEL_VISIBLE; }
  bool isGroupLeader() const { return flags & NODE_IS_GROUP_LEADER; }

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
edge_data_s() : weight(1.0f), tail(-1), head(-1), next_node_edge(-1), face(-1), next_face_edge(-1), arc(0), coverage(0) { }
edge_data_s(float _weight, int _tail, int _head, int _next_node_edge, int _face, int _next_face_edge, int _arc, long long _coverage = 0)
: weight(_weight), tail(_tail), head(_head), next_node_edge(_next_node_edge), face(_face), next_face_edge(_next_face_edge), arc(_arc), coverage(_coverage) { }
  
  float weight;
  int tail, head, next_node_edge, face, next_face_edge, arc, pair_edge = -1, parent_edge = -1;
  long long coverage;
};

struct face_data_s {
  // glm::vec3 normal;
  glm::vec2 centroid;
  graph_color_s color;
  Rect2d mbr;
  int first_edge;
  time_t timestamp;
  float sentiment;
  short feed, lang;
  long long app_id, filter_id;
  std::string label;
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
  void setLabel(const std::string & text) {
    if (label != text) {
      label = text;
      label_texture = 0;
    }
  }
  const std::string & getFaceLabel(int i) const { return label; }
};

#include "EdgeIterator.h"
#include "VisibleNodeIterator.h"

class GraphRefR;
class GraphRefW;
class Label;

class Graph : public GraphInterface {
 public:
  friend class GraphRefR;
  friend class GraphRefW;

  enum GraphStyle {
    DEFAULT_STYLE = 1,
    PACKED_SPHERES
  };
  
  Graph(int _id = 0);
  Graph(const Graph & other);
  Graph & operator= (const Graph & other) = delete;

  virtual ~Graph();  
  
  void updateEdgeWeight(int i, float d) {
    auto & attr = getEdgeAttributes(i);
    attr.weight += d;
    if (attr.weight > max_edge_weight) max_edge_weight = attr.weight;
    total_edge_weight += d;
  }

  std::vector<int> getLocationGraphs() const;
  std::vector<int> getNestedGraphIds() const;
  
  table::Table & getFaceData() { return faces; }
  const table::Table & getFaceData() const { return faces; }

  bool empty() const { return getEdgeCount() == 0; }
  // double calculateTotalEnergy() const;

  void addChild(int parent, int child);
  void addChild(int parent, int child, float dnodecomm);
  int removeChild(int child);
  int removeChild(int child, float dnodecomm);
  
  void removeAllChildren();

  bool isNodeVisible(int node) const;

  void setNodeAge(int node, float age) {
    if (node_geometry3.size() <= node) node_geometry3.resize(node + 1);
    node_geometry3[node].age = age;
  }

  void setIsGroupLeader(int node, bool t) {
    if (node_geometry3.size() <= node) node_geometry3.resize(node + 1);
    node_geometry3[node].setIsGroupLeader(t);
  }

  int getFaceFirstEdge(int i) const { return face_attributes[i].first_edge; }

  virtual void updateLocationGraph(int graph_id) { }
  virtual Graph * simplify() const { return 0; }
  virtual std::shared_ptr<Graph> createSimilar() const = 0;
  virtual Graph * copy() const = 0;
  virtual bool isDirected() const { return false; }

  bool hasPosition() const;
  bool applyFilter(time_t start_time, time_t end_time, float start_sentiment, float end_sentiment, Graph & source_graph, RawStatistics & stats, bool is_first_level, Graph * base_graph = 0);
  void reset();

  void extractLocationGraph(Graph & target_graph);

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

  int addEdge(int n1, int n2, int face = -1, float weight = 1.0f, int arc_id = 0, long long coverage = 0);

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
  virtual void updateMBR(int edge) { }
  virtual void addUniversalRegion() { }
  
  void getVisibleLabels(std::vector<Label> & labels) const;

  virtual std::set<int> getAdjacentRegions() const { return std::set<int>(); }

  bool hasEdge(int n1, int n2) const;

  int addNode(NodeType type = NODE_ANY) {
    int node_id = nodes->add(type);
    if (node_geometry3.size() <= node_id) node_geometry3.resize(node_id + 1);
    return node_id;
  }

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
  float weightedDegree(int n) override {
    if (node_geometry3.size() <= n) return 0.0f;
    return node_geometry3[n].weighted_indegree + node_geometry3[n].weighted_outdegree;    
  }

  unsigned int numberOfNeighbors(int n) override {
    if (node_geometry3.size() <= n) return 0;
    return node_geometry3[n].indegree + node_geometry3[n].outdegree;
  }

  float numberOfSelfLoops(int node) override {
    int edge = getNodeFirstEdge(node);
    float ws = 0.0f;
    while (edge != -1) {
      auto & ed = getEdgeAttributes(edge);
      if (ed.head == node) {
	ws += ed.weight;
      }
      edge = ed.next_node_edge;
    }
    return ws;
  }

  std::vector<std::pair<int, float> > getAllNeighbors(int node) const override;

#if 0
  size_t getNodeCount() const override { return getNodeArray().size(); }
#endif
  size_t getEdgeCount() const { return edge_attributes.size(); }
  size_t getFaceCount() const { return faces.size(); }  
  
  Rect2d & getFaceMBR(int i) { return face_attributes[i].mbr; }
  const glm::vec2 & getFaceCentroid(int i) const { return face_attributes[i].centroid; }
        
  bool hasNodeSelection() const { return has_node_selection; }
  void selectNodes(int node_id = -1, int depth = 0);
  
  void clearHighlight() {
    highlighted_node = -1;
  }

  void setKeywords(const std::string & k) { keywords = k; }
  const std::string & getKeywords() const { return keywords; }
  
  virtual void randomizeGeometry(bool use_2d = false);
  void refreshLayouts();

  void setId(int _id) { id = _id; }
  int getId() const { return id; }
  
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

  void setLocationGraphValid(bool t) { location_graph_valid = t; }
  bool isLocationGraphValid() const { return location_graph_valid; }

  virtual int addFace(time_t timestamp = 0, float sentiment = 0, short feed = 0, short lang = 0, long long app_id = -1, long long filter_id = -1) { // int shell1 = -1, int shell2 = -1) {
    int face_id = (int)face_attributes.size();
    face_attributes.push_back({ glm::vec2(0, 0), { 255, 255, 255, 255 }, Rect2d(), -1, timestamp, sentiment, feed, lang, app_id, filter_id, "", 0, 0 });
    faces.addRow();
    return face_id;
  }
  
  virtual bool checkConsistency() const { return true; }
          
  void setNodeTexture(const skey & key, int texture);
  
  void clearTextures(int clear_flags = CLEAR_ALL) {
    if (location_graph.get()) location_graph->clearTextures(clear_flags);
    nodes->clearTextures(clear_flags);
    for (auto & gd : nested_graphs) {
      gd.second->clearTextures(clear_flags);
    }
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

  void createClusters();
  void calculateEdgeCentrality();
      
  Graph & getActualGraph(float scale) {
    auto g = getFinal(scale);
    return g.get() ? *g : *this;
  }
  const Graph & getActualGraph(float scale) const {
    auto g = getFinal(scale);
    return g.get() ? *g : *this;
  }
  
  std::shared_ptr<Graph> getFinal(float scale);
  const std::shared_ptr<const Graph> getFinal(float scale) const;
  
  std::shared_ptr<Graph> & getLocation() { return location_graph; }
  const std::shared_ptr<const Graph> getLocation() const { return location_graph; }

  void addFinalGraph(std::shared_ptr<Graph> g) {
    final_graphs.push_back(g);
  }
  void setLocation(std::shared_ptr<Graph> g) { location_graph = g; }

  virtual void clear() {
    faces.clear();    
    face_attributes.clear();
    edge_attributes.clear();
    // do not clear nested graphs, unless we clear nodes also

    highlighted_node = -1;
    has_node_selection = false;
    total_indegree = total_outdegree = 0;
    total_edge_weight = total_weighted_indegree = total_weighted_outdegree = 0.0;
  }
      
  std::map<skey, int> & getFaceCache() { return face_cache; } 
  const std::map<skey, int> & getFaceCache() const { return face_cache; } 

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

  ConstVisibleNodeIterator begin_visible_nodes() const { return ConstVisibleNodeIterator(&(edge_attributes.front()), &(edge_attributes.back()) + 1, &(node_geometry3.front()), &(node_geometry3.back()) + 1, nodes->size()); }
  ConstVisibleNodeIterator end_visible_nodes() const { return ConstVisibleNodeIterator(); }
  
  int getGraphNodeId(int graph_id) const;
  bool updateSelection2(time_t start_time, time_t end_time, float start_sentiment, float end_sentiment);

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

  void updateNodeCoverage(int n, long long coverage) {
    if (node_geometry3.size() <= n) node_geometry3.resize(n + 1);
    auto & nd = node_geometry3[n];
    nd.coverage |= coverage;
    float new_weight = 0;
    for (int i = 0; i < 64; i++) {
      if (nd.coverage & (1 << i)) new_weight += 1.0f;
    }
    new_weight /= 64.0f;
    nd.coverage_weight = new_weight;
    if (new_weight > max_node_coverage_weight) max_node_coverage_weight = new_weight;
  }

  GraphRefR getGraphForReading(int graph_id, const char * debug_name) const;
  GraphRefW getGraphForWriting(int graph_id, const char * debug_name);

  std::string getGraphName(int graph_id) const {
    for (auto & gd : nested_graphs) {
      auto & graph = gd.second;
      if (graph->getId() == graph_id) {
	return nodes->getTable()["name"].getText(gd.first);
      }
    }   
    return "";
  }

  void setNestedGraph(int node_id, const std::shared_ptr<Graph> & graph) {
    nested_graphs[node_id] = graph;
  }
  
  const std::unordered_map<int, std::shared_ptr<Graph> > & getNestedGraphs() const { return nested_graphs; }
  void updateFaceAppearance();
    
  float getMinSignificance() const { return min_significance; }
  float getMinScale() const { return min_scale; }
  
  void setMinSignificance(float s) { min_significance = s; }
  void setMinScale(float s) { min_scale = s; }

  void relaxLinks(std::vector<node_position_data_s> & v) const;
  void applyGravity(float gravity, std::vector<node_position_data_s> & v) const;
  void applyDrag(RenderMode mode, float friction, std::vector<node_position_data_s> & v) const;
  void applyAge();

  double modularity() const; // calculate the modularity of the communities
  double modularityGain(int node, int comm, double dnodecomm, double w_degree) const;
  void initializeLouvain(int n);
  
  float getMaxNodeCoverageWeight() const { return max_node_coverage_weight; }

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

  void setInitialNodeAge(float t) { initial_node_age = t; }
  float getInitialNodeAge() const { return initial_node_age; }

  void setVersion(int _version) { version = _version; }
  void incVersion() { version++; }

  unsigned int calcVisibleNodeCount() const;
  
 protected:
  unsigned int getSuitableFinalGraphCount() const;
  Graph * getGraphById2(int id);
  const Graph * getGraphById2(int id) const;

  void incLabelVersion() { label_version++; }

  table::Table faces;
  std::vector<face_data_s> face_attributes;
  std::vector<edge_data_s> edge_attributes;
  double total_edge_weight = 0.0;
  float max_edge_weight = 0.0f, max_node_coverage_weight = 0.0f;
  std::shared_ptr<NodeArray> nodes;
 
 private:
  void lockReader() const { nodes->lockReader(); }
  void unlockReader() const { nodes->unlockReader(); }
  void lockWriter() { nodes->lockWriter(); }
  void unlockWriter() { nodes->unlockWriter(); }
  
  int version = 1, label_version = 1;
  short source_id = 0;

  int id;
  int highlighted_node = -1;
  unsigned int new_primary_objects_counter = 0, new_secondary_objects_counter = 0, new_images_counter = 0;
  bool location_graph_valid = false;
  std::shared_ptr<Graph> location_graph;
  std::vector<std::shared_ptr<Graph> > final_graphs;
  std::map<skey, int> face_cache;
  bool has_node_selection = false; 
  RawStatistics statistics;
  std::string keywords;
  int server_search_id = 0;
  bool is_loaded = false;
  float line_width = 1.0f;
  float min_significance = 0.0f, min_scale = 0.0f;
  std::vector<node_tertiary_data_s> node_geometry3;
  double total_weighted_outdegree = 0, total_weighted_indegree = 0;
  unsigned int total_outdegree = 0, total_indegree = 0;
  node_tertiary_data_s null_geometry3;
  int default_symbol_id = 0;
  bool show_nodes = true, show_edges = true, show_faces = true, show_labels = true;
  glm::vec4 node_color, edge_color, face_color;
  float initial_node_age = 0.0f;
  std::unordered_map<int, std::shared_ptr<Graph> > nested_graphs;
  
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
