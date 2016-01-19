#ifndef _GRAPH_H_
#define _GRAPH_H_

#include "MBRObject.h"
#include "NodeType.h"
#include "FeedMode.h"
#include "RenderMode.h"
#include "RawStatistics.h"
#include "LabelStyle.h"
#include "SizeMethod.h"
#include "LabelMethod.h"

#include "../system/Mutex.h"

#include <ArcData2D.h>
#include <Table.h>

#include <vector>
#include <set>

#include <glm/glm.hpp>

#define INITIAL_ALPHA		0.075f

#define DEFAULT_PROFILE	0
#define PENDING_PROFILE	1
#define BOT_PROFILE	2

#define NODE_SELECTED		1
#define NODE_LABEL_VISIBLE	2
#define NODE_FIXED_POSITION	4

#define CLEAR_LABELS	1
#define CLEAR_NODES	2
#define CLEAR_ALL	(CLEAR_LABELS | CLEAR_NODES)

class Graph;
class DisplayInfo;

#include "graph_color.h"

struct cluster_data_s {
  graph_color_s color;
  glm::vec3 position;
  glm::vec3 prev_position;
  unsigned int num_nodes;
};

struct node_data_s {
  graph_color_s color; // 0
  glm::uint32 normal; // 4
  glm::vec3 position; // 8
  glm::vec3 prev_position; // 20
  float age, size; // 32
  short texture, flags; // 40
};

struct node_secondary_data_s {
  NodeType type;
  int first_edge, orig_node_id;
  int cluster_id;
  short label_texture;
  unsigned short label_visibility_val;
  float indegree, outdegree;
  std::string label;
  std::shared_ptr<Graph> nested_graph;
};

struct edge_data_s {
edge_data_s() : weight(1.0f), tail(-1), head(-1), next_node_edge(-1), face(-1), next_face_edge(-1), arc(0) { }
edge_data_s(float _weight, int _tail, int _head, int _next_node_edge, int _face, int _next_face_edge, int _arc)
: weight(_weight), tail(_tail), head(_head), next_node_edge(_next_node_edge), face(_face), next_face_edge(_next_face_edge), arc(_arc) { }

  float weight;
  int tail, head, next_node_edge, face, next_face_edge, arc;
};

struct face_data_s {
  glm::vec3 normal;
  glm::vec2 centroid;
  Rect2d mbr;
  int first_edge, next_face_in_region, region;
  time_t timestamp;
  float sentiment;
  short feed, lang;
  long long app_id;
  long long filter_id;
};

struct region_data_s {
  graph_color_s color;
  std::string label;
  Rect2d mbr;
  int first_face;
};

class EdgeIterator {
 public:
 EdgeIterator() : ptr(0), stride(0) { }
  EdgeIterator( edge_data_s * _ptr, size_t _stride )
    : ptr((unsigned char*)_ptr), stride(_stride) { }

  bool isValid() const { return ptr != 0; }

  edge_data_s & operator*() { return *(edge_data_s*)ptr; }
  edge_data_s * operator->() { return (edge_data_s*)ptr; }
  edge_data_s * get() { return (edge_data_s*)ptr; }

  EdgeIterator & operator++() { ptr += stride; return *this; }
  EdgeIterator & operator--() { ptr -= stride; return *this; }
  
  bool operator==(const EdgeIterator & other) const { return this->ptr == other.ptr; }
  bool operator!=(const EdgeIterator & other) const { return this->ptr != other.ptr; }

  void clear() { ptr = 0; stride = 0; }

 private:
  unsigned char * ptr;
  size_t stride;
};

class ConstEdgeIterator {
 public:
 ConstEdgeIterator() : ptr(0), stride(0) { }
 ConstEdgeIterator( const edge_data_s * _ptr, size_t _stride )
    : ptr((const unsigned char*)_ptr), stride(_stride) { }

  bool isValid() const { return ptr != 0; }

  const edge_data_s & operator*() const { return *(edge_data_s*)ptr; }
  const edge_data_s * operator->() const { return (edge_data_s*)ptr; }
  const edge_data_s * get() const { return (edge_data_s*)ptr; }

  ConstEdgeIterator & operator++() { ptr += stride; return *this; }
  ConstEdgeIterator & operator--() { ptr -= stride; return *this; }
  
  bool operator==(const ConstEdgeIterator & other) const { return this->ptr == other.ptr; }
  bool operator!=(const ConstEdgeIterator & other) const { return this->ptr != other.ptr; }

  void clear() { ptr = 0; stride = 0; }

 private:
  const unsigned char * ptr;
  size_t stride;
};

#include "StatusInterface.h"
#include "GraphElement.h"

#include <string>
#include <vector>
#include <list>
#include <set>

#include "skey.h"

#define GF_TEMPORAL_GRAPH	1
#define GF_HAS_SPATIAL_DATA	4
#define GF_HAS_ARC_DATA		8
#define GF_COMPLEX		16
#define GF_HAS_TEXTURES		32
#define GF_PER_NODE_COLORS	64
#define GF_HAS_SUBGRAPHS	128
#define GF_DOUBLE_BUFFERED_VBO	256

class VBO;
class TextureAtlas;

namespace canvas {
  class ContextFactory;
  class Color;
};

#if 0
class Cursor {
 public:
  
  int node_idx;
};
#endif

#if 0
class Community {
 public:
  Community() { }
  
 private:
  std::vector<int> nodes;
};
#endif

class GraphRefR;
class GraphRefW;

class Graph : public MBRObject {
 public:
  friend class GraphRefR;
  friend class GraphRefW;

  enum Personality {
    NONE = 0,
    SOCIAL_MEDIA,
    TIME_SERIES
  };
  enum GraphStyle {
    DEFAULT_STYLE = 1,
    PACKED_SPHERES
  };
  enum WeightMethod {
    NO_WEIGHT = 0,
    CENTRALITY
  };
  
  Graph(int _dimensions, int _id = 0);
  Graph(const Graph & other);
  Graph & operator= (const Graph & other) = delete;

  virtual ~Graph();  
  
  std::string getEdgeId(int i) { return edges["id"].getText(i); }

  void updateEdgeWeight(int i, float d) {
    auto & attr = getEdgeAttributes(i);
    attr.weight += d;
    total_edge_weight += fabsf(d);
  }
  void setPersonality(Personality _personality) { personality = _personality; }
  Personality getPersonality() const { return personality; }

  std::vector<int> getLocationGraphs() const;
  std::vector<int> getNestedGraphIds() const;

  table::Table & getClusterData() { return clusters; }
  
  table::Table & getEdgeData() { return edges; }
  const table::Table & getEdgeData() const { return edges; }

  table::Table & getFaceData() { return faces; }
  const table::Table & getFaceData() const { return faces; }

  table::Table & getRegionData() { return regions; }
  table::Table & getShellData() { return shells; }

  bool empty() const { return getNodeCount() == 0; }
  
  // double calculateTotalEnergy() const;

  std::string getGraphName(int graph_id) const {
    for (int i = 0; i < getNodeCount(); i++) {
      auto & graph = node_geometry2[i].nested_graph;
      if (graph.get() && graph->getId() == graph_id) {
	return getNodeData()["name"].getText(i);
      }
    }   
    return "";
  }

  int getNodeFirstEdge(int i) const { return node_geometry2[i].first_edge; }
  int getFaceFirstEdge(int i) const { return face_attributes[i].first_edge; }
  int getFaceRegion(int i) const { return face_attributes[i].region; }

  virtual void updateLocationGraph(int graph_id) { }
  virtual Graph * simplify() const { return 0; }
  virtual std::shared_ptr<Graph> createSimilar() const = 0;
  virtual Graph * copy() const = 0;
  virtual bool hasPosition() const { return false; }
  virtual bool isDirected() const { return false; }
  virtual bool updateData(time_t start_time, time_t end_time, float start_sentiment, float end_sentiment, Graph & source_graph, RawStatistics & stats) { return false; }
  virtual void reset() { }
  void extractLocationGraph(Graph & target_graph);

  int getNextNodeEdge(int edge) const {
    return getEdgeAttributes(edge).next_node_edge;
  }
  int getEdgeTargetNode(int edge) const {
    return getEdgeAttributes(edge).head;
  }
  virtual int addEdge(int n1, int n2, int face = -1, float weight = 1.0f, int arc_id = 0) = 0;
  virtual void setEdgeFace(int edge1, int face) { }
  
  virtual int getNextFaceEdge(int edge) const { return -1; }
  virtual int getEdgeRightFace(int edge) const { return -1; }
  virtual int getEdgeLeftFace(int edge) const { return -1; }
  virtual int addRegion() { return -1; }
  virtual int getRegionFirstFace(int region) const { return -1; }
  virtual int getRegionNextFace(int region, int face) const { return -1; }
  
  virtual void calculateRegionAreas() { }
  virtual void mapFacesToNodes(Graph & target) { }
  virtual void mapRegionsToNodes(Graph & target) { }
  virtual void colorizeRegions() { }
  virtual void spatialAggregation(const Graph & other) { }
  virtual void updateMBR(int edge) { }
  virtual void addUniversalRegion() { }
  
  void createEdgeVBO(VBO & vbo, bool is_spherical, float earth_radius) const;
  void createRegionVBO(VBO & vbo, bool spherical, float earth_radius) const;
  void createNodeVBOForSprites(VBO & vbo, bool is_spherical, float earth_radius) const;
  void createNodeVBOForQuads(VBO & vbo, const TextureAtlas & atlas, float node_scale, bool is_spherical, float earth_radius) const;
  void createLabelVBO(VBO & vbo, const TextureAtlas & atlas, float node_scale) const;

  virtual std::set<int> getAdjacentRegions() const { return std::set<int>(); }

  bool hasEdge(int n1, int n2) const;

#if 0
  void setEdgeId(int edge, const std::string & edge_id) {
    getEdgeData().addTextColumn("id").setValue(edge, edge_id);
  }

  void setFaceId(int face, const std::string & face_id) {
    getFaceData().addTextColumn("id").setValue(face, face_id);
  }

  void setRegionId(int region, const std::string & region_id) {
    getRegionData().addTextColumn("id").setValue(region, region_id);
  }
#endif

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
 
  void simplifyWithClusters(const std::vector<int> & clusters, Graph & target_graph);

  // return the weighted degree of the node
  double weighted_degree(int node) const {
#if 1
    double d = 0;
    int edge = getNodeFirstEdge(node);
    while (edge != -1) {
      d += getEdgeAttributes(edge).weight;
      edge = getNextNodeEdge(edge);
    }
    return d;
#else
    if (weights.size() == 0) {
      return (double)nb_neighbors(node);
    } else {
      pair<vector<unsigned int>::iterator, vector<float>::iterator > p = neighbors(node);
      double res = 0;
      for (unsigned int i=0 ; i<nb_neighbors(node) ; i++) {
	res += (double)*(p.second+i);
      }
      return res;
    }
#endif
  }

  double nb_selfloops(int node) const {
#if 1
    int edge = getNodeFirstEdge(node);
    while (edge != -1) {
      if (getEdgeTargetNode(edge) == node) {
	return getEdgeAttributes(edge).weight;
      }
    }
    return 0;
#else
    pair<vector<unsigned int>::iterator, vector<float>::iterator > p = neighbors(node);
    for (unsigned int i=0 ; i<nb_neighbors(node) ; i++) {
      if (*(p.first+i)==node) {
	if (weights.size()!=0)
	  return (double)*(p.second+i);
	else 
	  return 1.;
      }
    }
    return 0.;
#endif
  }
 
  // int getTotalDegrees(int i) const { return 1; }; //return getInDegrees() + getOutDegrees(); }

  size_t getClusterCount() const { return clusters.size(); }
  size_t getEdgeCount() const { return edges.size(); }
  size_t getFaceCount() const { return faces.size(); }  
  size_t getRegionCount() const { return regions.size(); }
  size_t getShellCount() const { return shells.size(); }
  
  void updateNodeAppearanceSlow(int node_id);
  void updateAppearance();
  const std::string & getNodeLabel(int i) const { return node_geometry2[i].label; }
  const std::string & getRegionLabel(int i) const { return region_attributes[i].label; }
  glm::dvec3 getRegionPosition(int i) {
    return region_attributes[i].mbr.getCenter();
  }
  Rect2d & getRegionMBR(int i) { return region_attributes[i].mbr; }
  Rect2d & getFaceMBR(int i) { return face_attributes[i].mbr; }
  const glm::vec2 & getFaceCentroid(int i) const { return face_attributes[i].centroid; }

  void setNodeLabel(int i, const std::string & text) {
    if (node_geometry2[i].label != text) {
      node_geometry2[i].label = text;
      node_geometry2[i].label_texture = 0;
    }
  }

  void setNestedGraph(int i, std::shared_ptr<Graph> graph) {
    node_geometry2[i].nested_graph = graph;
  }
  
  void clearLabelTexture(int i) { node_geometry2[i].label_texture = 0; }
      
  bool hasNodeSelection() const { return has_node_selection; }
  void selectNodes(int node_id = -1, int depth = 0);

  void setNodeColorByColumn(int column);
  void setRegionColorByColumn(int column);

  void setClusterVisibility(bool t) { show_clusters = t; }
  void setNodeVisibility(bool t) { show_nodes = t; }
  void setEdgeVisibility(bool t) { show_edges = t; }
  void setRegionVisibility(bool t) { show_regions = t; }
  void setLabelVisibility(bool t) { show_labels = t; }

  bool getClusterVisibility() const { return show_clusters; }
  bool getNodeVisibility() const { return show_nodes; }
  bool getEdgeVisibility() const { return show_edges; }
  bool getRegionVisibility() const { return show_regions; }
  bool getLabelVisibility() const { return show_labels; }
  
  const graph_color_s & getRegionColor(int i) const { return region_attributes[i].color; }
  void setRegionColor(int i, const graph_color_s & c) { region_attributes[i].color = c; }
  void setFaceNormal(int i, const glm::vec3 & n) { face_attributes[i].normal = n; }
  void setFaceCentroid(int i, const glm::vec2 & c) { face_attributes[i].centroid = c; }
  
  int getHighlightedRegion() const { return highlighted_region; }
  void highlightRegion(int i) { highlighted_region = i; }
  void clearHighlight() {
    highlighted_node = highlighted_region = -1;
  }

  void setKeywords(const std::string & k) { keywords = k; }
  const std::string & getKeywords() const { return keywords; }
  
  const glm::vec4 & getDefaultNodeColor() const { return node_color; }
  const glm::vec4 & getDefaultEdgeColor() const { return edge_color; }
  const glm::vec4 & getDefaultRegionColor() const { return region_color; }
  
  void setDefaultNodeColor(const glm::vec4 & color) { node_color = color; }
  void setDefaultEdgeColor(const glm::vec4 & color) { edge_color = color; }
  void setDefaultRegionColor(const glm::vec4 & color) { region_color = color; }

  bool testFlags(unsigned int bit) const { return (flags & bit) != 0; }
  Graph & updateFlags(unsigned int bit, bool t) { flags = (t ? flags | bit : flags & ~bit); return *this; }

  bool isTemporal() const { return testFlags(GF_TEMPORAL_GRAPH); }
  Graph & setTemporal(bool t) { return updateFlags(GF_TEMPORAL_GRAPH, t); } 
  
  bool isComplexGraph() const { return testFlags(GF_COMPLEX); }
  Graph & setComplexGraph(bool t) { return updateFlags(GF_COMPLEX, t); }

  bool hasSpatialData() const { return testFlags(GF_HAS_SPATIAL_DATA); }
  Graph & setHasSpatialData(bool t) { return updateFlags(GF_HAS_SPATIAL_DATA, t); }

  bool hasArcData() const { return testFlags(GF_HAS_ARC_DATA); }
  Graph & setHasArcData(bool t) { return updateFlags(GF_HAS_ARC_DATA, t); }

  bool hasTextures() const { return testFlags(GF_HAS_TEXTURES); }
  Graph & setHasTextures(bool t) { return updateFlags(GF_HAS_TEXTURES, t); }

  bool hasDoubleBufferedVBO() const { return testFlags(GF_DOUBLE_BUFFERED_VBO); }
  Graph & setDoubleBufferedVBO(bool t) { return updateFlags(GF_DOUBLE_BUFFERED_VBO, t); }

  void setPerNodeColors(bool t) { updateFlags(GF_PER_NODE_COLORS, t); }
  bool perNodeColorsEnabled() const { return testFlags(GF_PER_NODE_COLORS); }

  void setHasSubGraphs(bool t) { updateFlags(GF_HAS_SUBGRAPHS, t); }
  bool hasSubGraphs() const { return testFlags(GF_HAS_SUBGRAPHS); }

  virtual void randomizeGeometry(bool use_2d = false);
  void refreshLayouts();
  std::vector<std::shared_ptr<Graph> > getNestedGraphs();

  void setId(int _id) { id = _id; }
  int getId() const { return id; }
  
  void setSourceId(short s) { source_id = s; }
  short getSourceId() const { return source_id; }

  void setServerSearchId(int id) { server_search_id = id; }
  int getServerSearchId() const { return server_search_id; }
  bool isLoaded() const { return is_loaded; }
  void setIsLoaded(bool t) { is_loaded = t; }
  
  void setAlpha3(float f) { alpha = f; }

  void updateAlpha() {
    alpha *= 0.99f;
  }
  float getAlpha2() const { return alpha; }
  void resume2() {
    alpha = INITIAL_ALPHA;
  }
  void stop() { alpha = 0.0f; }

  unsigned int getNewPrimaryObjects() const { return new_primary_objects_counter; }
  unsigned int getNewSecondaryObjects() const { return new_secondary_objects_counter; }
  unsigned int getNewImages() const { return new_images_counter; }
  unsigned int getNewObjects(FeedMode feed) const { return feed == PRIMARY_FEED ? new_primary_objects_counter : (feed == SECONDARY_FEED ? new_secondary_objects_counter : new_images_counter); }
  void updateNewPrimaryObjects(unsigned int i) { new_primary_objects_counter += i; }
  void updateNewSecondaryObjects(unsigned int i) { new_secondary_objects_counter += i; }
  void resetNewObjects2() { new_primary_objects_counter = new_secondary_objects_counter = new_images_counter = 0; }

  void setLocationGraphValid(bool t) { location_graph_valid = t; }
  bool isLocationGraphValid() const { return location_graph_valid; }

  void relaxLinks();
  void applyGravity(float gravity);
  void applyDragAndAge(RenderMode mode, float friction);

  int getSRID() const { return srid; }
  void setSRID(int _srid) { srid = _srid; }

  virtual int addCluster() {
    int cluster_id = cluster_attributes.size();
    cluster_attributes.push_back({ { 127, 127, 127, 127 }, glm::vec3(), glm::vec3(), 0 });
    while (clusters.size() < cluster_attributes.size()) {
      clusters.addRow();
    }
    return cluster_id;
  }
  
  virtual int addNode(NodeType type = NODE_ANY, float size = 0.0f, float age = 0.0f) {
    int node_id = node_geometry.size();
    node_geometry.push_back({ { 200, 200, 200, 255 }, 0, glm::vec3(), glm::vec3(), age, size, 0, NODE_SELECTED });
    node_geometry2.push_back({ type, -1, -1, -1, 0, 0, 0, 0, "", std::shared_ptr<Graph>() });
    version++;
    while (nodes.size() < node_geometry.size()) {
      nodes.addRow();
    }
    updateNodeSize(node_id);
    return node_id;
  }

  virtual int addFace(int region_id = -1, time_t timestamp = 0, float sentiment = 0, short feed = 0, short lang = 0, long long app_id = -1, long long filter_id = -1, int shell1 = -1, int shell2 = -1) {
    int face_id = (int)face_attributes.size();
    int next_face_in_region = -1;
    if (region_id >= 0) {
      next_face_in_region = region_attributes[region_id].first_face;
      region_attributes[region_id].first_face = face_id;
    }
    face_attributes.push_back({ glm::vec3(1, 0, 0), glm::vec2(0, 0), Rect2d(), -1, next_face_in_region, region_id, timestamp, sentiment, feed, lang, app_id, filter_id });
    faces.addRow();
    return face_id;
  }
  
  virtual bool checkConsistency() const { return true; }
  
  std::vector<int> createSortedNodeIndices(const glm::vec3 & camera_pos) const;

  std::vector<int> addNodes(size_t n) {
    std::vector<int> v;
    while (n--) {
      v.push_back(addNode());
    }
    return v;
  }
  
  const glm::vec3 & getPosition(int i) const {
    return node_geometry[i].position;
  }
  const glm::vec3 & getPrevPosition(int i) const {
    return node_geometry[i].prev_position;
  }
  const std::pair<glm::vec3, glm::vec3> getPositions(int i) const {
    return std::pair<glm::vec3, glm::vec3>(node_geometry[i].position, node_geometry[i].prev_position);
  }

  node_data_s * getGeometryPtr() {
    node_data_s & a = node_geometry.front();
    return &a;
  }
  std::vector<node_data_s> & getGeometry() { return node_geometry; }
  std::vector<node_secondary_data_s> & getGeometry2() { return node_geometry2; }
    
#if 0
  int addNodeWithId(const std::string & id_text) {
    int node_id = addNode();
    getNodeData().addTextColumn("id").setValue(node_id, id_text);
    return node_id;
  }
  void setNodeId(int node, const std::string & id) {
    getNodeData().addTextColumn("id").setValue(node, id);
  }
#endif
    
  void updatePosition(int i, const glm::vec3 & v) {
    node_geometry[i].position = v;    
  }
  void updatePrevPosition(int i, const glm::vec3 & v) {
    node_geometry[i].prev_position = v;    
  }
  void setPosition(int i, const glm::vec3 & v) { 
    node_geometry[i].position = node_geometry[i].prev_position = v;
    mbr.growToContain(v.x, v.y);
    version++;
  }
  void setPosition(int i, const glm::vec2 & v) {
    setPosition(i, glm::vec3(v, 0.0f));
  }
  void setPositions(int i, const std::pair<glm::vec3, glm::vec3> & p) {
    node_geometry[i].position = p.first;
    node_geometry[i].prev_position = p.second;
    mbr.growToContain(p.first.x, p.first.y);
    version++;
  }
  void setNormal(int i, const glm::vec4 & v);  
  void setNodeColor2(int i, const graph_color_s & c) {
    node_geometry[i].color = c;
    version++;
  }
  void setNodeColor2(int i, const canvas::Color & c);
  void setClusterColor(int i, const graph_color_s & c) {
    cluster_attributes[i].color = c;
    version++;
  }
  void setClusterColor(int i, const canvas::Color & c);

  void setNodeTexture(int i, int texture) {
    node_geometry[i].texture = texture;
    version++;
  }
  void setNodeTexture(const skey & key, int texture);
  
  void clearTextures(int clear_flags = CLEAR_ALL) {
    if (final_graph.get()) final_graph->clearTextures(clear_flags);
    if (location_graph.get()) location_graph->clearTextures(clear_flags);
    for (int i = 0; i < getNodeCount(); i++) {
      if (clear_flags & CLEAR_LABELS) {
	node_geometry2[i].label_texture = 0;
	node_geometry2[i].label_visibility_val = 0;
	node_geometry[i].flags &= ~NODE_LABEL_VISIBLE;
      }
      if (clear_flags & CLEAR_NODES) {
	node_geometry[i].texture = DEFAULT_PROFILE;
      }
      if (node_geometry2[i].nested_graph.get()) {
	node_geometry2[i].nested_graph->clearTextures(clear_flags);
      }
    }
    version++;
  }
  void setLabelTexture(int i, int texture) {
    node_geometry2[i].label_texture = texture;
    version++;
  }
  void setLabelTexture(const skey & key, int texture);  
  void setNodeLabelVisibilityValue(int i, float f) {
    int f2 = int(f * 65535);
    if (f2 < 0) f2 = 0;
    else if (f2 > 65535) f2 = 65535;
    node_geometry2[i].label_visibility_val = (unsigned short)f2;
  }
  bool setNodeLabelVisibility(int i, bool t) {
    bool orig_t = node_geometry[i].flags | NODE_LABEL_VISIBLE ? true : false;
    if (t != orig_t || 1) {
      if (t) {
	node_geometry[i].flags |= NODE_LABEL_VISIBLE;
      } else {
	node_geometry[i].flags &= ~NODE_LABEL_VISIBLE;
      }
      return true;
    } else {
      return false;
    }
  }
  bool updateNodeLabelValues(int i, float visibility) {
    float vv = getNodeLabelVisibilityValue(i) + visibility;
    if (vv < 0) vv = 0;
    else if (vv > 1) vv = 1;
    setNodeLabelVisibilityValue(i, vv);
    if (vv >= 0.75) return setNodeLabelVisibility(i, true);
    else if (vv <= 0.25) return setNodeLabelVisibility(i, false);
    else return false;
  }
  void setNodeFixedPosition(int i, bool t) {
    if (t) {
      node_geometry[i].flags |= NODE_FIXED_POSITION;
    } else {
      node_geometry[i].flags &= ~NODE_FIXED_POSITION;
    }
    // doesn't affect anything directly, so no need to update version
  }

  const node_data_s & getNodeData(int i) const { return node_geometry[i]; }
  node_data_s & getNodeData(int i) { return node_geometry[i]; }

  const node_secondary_data_s & getNodeSecondaryData(int i) const { return node_geometry2[i]; }
  node_secondary_data_s & getNodeSecondaryData(int i) { return node_geometry2[i]; }
  
  const graph_color_s & getNodeColor(int i) const { return node_geometry[i].color; }
  bool getNodeLabelVisibility(int i) const { return node_geometry[i].flags & NODE_LABEL_VISIBLE ? true : false; }
  float getNodeLabelVisibilityValue(int i) const { return node_geometry2[i].label_visibility_val / 65535.0f; }

  int getNodeCount() const { return node_geometry.size(); }

  // std::string getNodeId(int i) const { return nodes["id"].getText(i); }
  std::string getNodeName(int i) const { return nodes["name"].getText(i); }
  std::string getNodeUsername(int i) const { return nodes["uname"].getText(i); }
  
  table::Table & getNodeData() { return nodes; }
  const table::Table & getNodeData() const { return nodes; }
    
  int getDimensions() const { return dimensions; }
  int getVersion() const { return version; }

  void createClusters();
  void calculateEdgeCentrality();
  
  void incVersion() { version++; }
  void setVersion(int _version) { version = _version; }
    
  void setNodeSizeMethod(SizeMethod m) { node_size_method = m; }
  const SizeMethod & getNodeSizeMethod() const { return node_size_method; }

  void setLabelMethod(const LabelMethod & m) { label_method = m; }
  const LabelMethod & getLabelMethod() const { return label_method; }

  Graph & getActualGraph(float scale) {
    auto g = getFinal(scale);
    return g.get() ? *g : *this;
  }
  const Graph & getActualGraph(float scale) const {
    auto g = getFinal(scale);
    return g.get() ? *g : *this;
  }
  
  std::shared_ptr<Graph> getFinal(float scale) {
    return final_graph;
  }
  const std::shared_ptr<const Graph> getFinal(float scale) const {
    return final_graph;
  }
  
  std::shared_ptr<Graph> & getLocation() { return location_graph; }
  const std::shared_ptr<const Graph> getLocation() const { return location_graph; }
  std::shared_ptr<Graph> & getSimplified() { return simplified_graph; }

  void setFinal(std::shared_ptr<Graph> g) { final_graph = g; }
  void setLocation(std::shared_ptr<Graph> g) { location_graph = g; }
  void setSimplified(std::shared_ptr<Graph> g) { simplified_graph = g; } 

  virtual void clear() {
    clusters.clear();
    edges.clear();
    faces.clear();    
    regions.clear();
    shells.clear();
    cluster_attributes.clear();
    face_attributes.clear();
    region_attributes.clear();
    node_geometry.clear();
    node_geometry2.clear();
    nodes.clear();

    highlighted_node = -1;
    highlighted_region = -1;
    node_cache.clear();
    has_node_selection = false;
    total_indegree = total_outdegree = 0.0;
    total_edge_weight = 0.0;
  }
      
  std::map<skey, int> & getNodeCache() { return node_cache; } 
  const std::map<skey, int> & getNodeCache() const { return node_cache; } 

  std::map<skey, int> & getFaceCache() { return face_cache; } 
  const std::map<skey, int> & getFaceCache() const { return face_cache; } 

  int getNodeId(short source_id, long long source_object_id) const;
  int getFaceId(short source_id, long long source_object_id) const;
  
  int pickNode(const DisplayInfo & display, int x, int y, float node_scale) const;

  void setNodeCluster(int node_id, int cluster_id) {
    assert(node_id >= 0 && node_id < getNodeCount());
    auto & nd = getNodeSecondaryData(node_id);
    if (nd.cluster_id != -1) {
      assert(nd.cluster_id >= 0 && nd.cluster_id <= (int)getClusterCount());
      getClusterAttributes(nd.cluster_id).num_nodes--;
    }
    nd.cluster_id = cluster_id;
    assert(nd.cluster_id >= 0 && nd.cluster_id <= (int)getClusterCount());
    getClusterAttributes(nd.cluster_id).num_nodes++;
  }
  
  cluster_data_s & getClusterAttributes(int i) { return cluster_attributes[i]; }
  const cluster_data_s & getClusterAttributes(int i) const { return cluster_attributes[i]; }

  face_data_s & getFaceAttributes(int i) { return face_attributes[i]; }
  const face_data_s & getFaceAttributes(int i) const { return face_attributes[i]; }

  virtual edge_data_s & getEdgeAttributes(int i) = 0;
  virtual const edge_data_s & getEdgeAttributes(int i) const = 0;  

  virtual EdgeIterator begin_edges() = 0;
  virtual EdgeIterator end_edges() = 0;
  virtual ConstEdgeIterator begin_edges() const = 0;
  virtual ConstEdgeIterator end_edges() const = 0;

#if 0
  NodeIterator begin_nodes() { return NodeIterator(&(node_geometry.front())); }
  NodeIterator end_nodes() { return NodeIterator(&(node_geometry.back())) + 1; }
#endif

  int getGraphNodeId(int graph_id) const;
  void storeChangesFromFinal();
  bool updateSelection2(time_t start_time, time_t end_time, float start_sentiment, float end_sentiment);

  const RawStatistics & getStatistics() const { return statistics; }
  RawStatistics & getStatistics() { return statistics; }

  float getLineWidth() const { return line_width; }
  void setLineWidth(float w) { line_width = w; }

  bool updateLabelVisibility(const DisplayInfo & display, bool reset = false);

  void setLabelStyle(LabelStyle style) { label_style = style; }
  LabelStyle getLabelStyle() const { return label_style; }

  void setDefaultSymbolId(int symbol_id) { default_symbol_id = symbol_id; }
  int getDefaultSymbolId() const { return default_symbol_id; }

  void setRadius(float r) { radius = r; }
  float getRadius() const { return radius; }

  int addArcGeometry(const ArcData2D & data) {
    int arc_id = 1 + int(arc_geometry.size());
    arc_geometry.push_back(data);
    return arc_id;
  }
  const std::vector<ArcData2D> & getArcGeometry() const { return arc_geometry; }

  skey getNodeKey(int node_id) const;
      
  void invalidateVisibleNodes();

  GraphRefR getGraphForReading(int graph_id) const;
  GraphRefW getGraphForWriting(int graph_id);

 protected:
  Graph * getGraphById2(int id);
  const Graph * getGraphById2(int id) const;
  void updateNodeSize(int n) { node_geometry[n].size = node_size_method.calculateSize(node_geometry2[n], total_indegree, total_outdegree, getNodeCount() ); }

  table::Table clusters, nodes, edges, faces, regions, shells;
  std::vector<cluster_data_s> cluster_attributes;
  std::vector<face_data_s> face_attributes;
  std::vector<region_data_s> region_attributes;
  std::vector<node_data_s> node_geometry;
  std::vector<node_secondary_data_s> node_geometry2;
  double total_edge_weight = 0;
  double total_outdegree = 0, total_indegree = 0;
 
 private:
  void lockReader() const {
    MutexLocker locker(mutex);
    num_readers++;
    if (num_readers == 1) writer_mutex.lock();
  }
  void unlockReader() const {
    MutexLocker locker(mutex);
    num_readers--;
    if (num_readers == 0) {
      writer_mutex.unlock();
    }
  }
  void lockWriter() {
    writer_mutex.lock();
  }
  void unlockWriter() {
    writer_mutex.unlock();
  }

  int srid = 0, version = 1;
  int dimensions;
  int highlighted_node = -1, highlighted_region = -1;
  int id;
  bool show_clusters = true, show_nodes = true, show_edges = true, show_regions = true, show_labels = true;
  glm::vec4 node_color, edge_color, region_color;
  unsigned int flags = 0;
  short source_id = 0;
  float alpha = 0.0f;
  unsigned int new_primary_objects_counter = 0, new_secondary_objects_counter = 0, new_images_counter = 0;
  bool location_graph_valid = false;
  std::shared_ptr<Graph> final_graph, location_graph, simplified_graph;
  std::map<skey, int> node_cache, face_cache;
  bool has_node_selection = false; 
  Personality personality = NONE;
  RawStatistics statistics;
  SizeMethod node_size_method;
  std::string node_color_column;
  LabelMethod label_method;
  std::string keywords;
  int server_search_id = 0;
  bool is_loaded = false;
  float line_width = 1.0f;
  LabelStyle label_style = LABEL_PLAIN;
  int default_symbol_id = 0;
  float radius = 0.0f;
  std::vector<ArcData2D> arc_geometry;

  mutable int num_readers = 0;
  mutable Mutex mutex, writer_mutex;
  
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
  ~GraphRefR() {
    if (graph) graph->unlockReader();
  }
  GraphRefR & operator=(const GraphRefR & other) {
    if (&other != this) {
      if (graph) graph->unlockReader();
      graph = other.graph;
      if (graph) graph->lockReader();
    }
    return *this;
  }
  const Graph * operator->() const {
    return graph;
  }
  const Graph & operator*() const {
    return *graph;
  }
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
  ~GraphRefW() {
    if (graph) graph->unlockWriter();
  }
  GraphRefW & operator=(const GraphRefW & other) = delete;
  
  Graph * operator->() {
    return graph;
  }
  Graph & operator*() {
    return *graph;
  }
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
