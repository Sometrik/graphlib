#ifndef _NODEARRAY_H_
#define _NODEARRAY_H_

#include <skey.h>

#include "ReadWriteObject.h"
#include "NodeType.h"
#include "SizeMethod.h"
#include "LabelStyle.h"
#include "RenderMode.h"
#include "LabelMethod.h"
#include "ArcData2D.h"
#include "Table.h"

#include <glm/glm.hpp>
#include <string>

#define GF_TEMPORAL_GRAPH	1
#define GF_HAS_SPATIAL_DATA	4
#define GF_HAS_ARC_DATA		8
#define GF_DYNAMIC		16
#define GF_HAS_TEXTURES		32
#define GF_PER_NODE_COLORS	64
#define GF_TEMPORAL_COVERAGE	128
#define GF_FLATTEN_HIERARCHY	256

#define DEFAULT_PROFILE	0
#define PENDING_PROFILE	1
#define BOT_PROFILE	2

#define CLEAR_LABELS	1
#define CLEAR_NODES	2
#define CLEAR_ALL	(CLEAR_LABELS | CLEAR_NODES)

struct node_position_data_s {
  glm::vec3 position, prev_position;
};

struct node_data_s {
  glm::vec3 position;
  glm::vec3 prev_position;
  short texture, label_texture;
  NodeType type;
  std::string label;
  int group_node;
};

class NodeArray : public ReadWriteObject {
 public:
  enum Personality {
    NONE = 0,
    SOCIAL_MEDIA,
    TIME_SERIES
  };

  NodeArray();
  NodeArray(const NodeArray & other) = delete;
  NodeArray & operator=(const NodeArray & other) = delete;

  size_t size() const { return nodes.size(); }
  bool empty() const { return nodes.empty(); }
  
  table::Table & getTable() { return nodes; }
  const table::Table & getTable() const { return nodes; }

  void setPersonality(Personality _personality) { personality = _personality; }
  Personality getPersonality() const { return personality; }

  void setNodeSizeMethod(const SizeMethod & m) { size_method = m; }
  const SizeMethod & getNodeSizeMethod() const { return size_method; }

  void setLabelMethod(const LabelMethod & m) { label_method = m; }
  const LabelMethod & getLabelMethod() const { return label_method; }

  void updateNodeAppearance(int node_id);

  std::map<skey, int> & getNodeCache() { return node_cache; } 
  const std::map<skey, int> & getNodeCache() const { return node_cache; } 

  int add(NodeType type = NODE_ANY) {
    int node_id = node_geometry.size();
    node_geometry.push_back({ glm::vec3(), glm::vec3(), 0, 0, type, "", -1 });
    version++;
    while (nodes.size() < node_geometry.size()) {
      nodes.addRow();
    }
    return node_id;
  }

  int getNodeId(short source_id, long long source_object_id) const {
    auto it = getNodeCache().find(skey(source_id, source_object_id));
    if (it != getNodeCache().end()) {
      return it->second;
    }
    return -1;
  }

  void updatePosition(int i, const glm::vec3 & v) {
    node_geometry[i].position = v;    
  }
  void setPosition(int i, const glm::vec3 & v) { 
    node_geometry[i].position = node_geometry[i].prev_position = v;
    // mbr.growToContain(v.x, v.y);
    version++;
  }
  void setPosition(int i, const glm::vec2 & v) {
    setPosition(i, glm::vec3(v, 0.0f));
  }
  void setPositions(int i, const std::pair<glm::vec3, glm::vec3> & p) {
    node_geometry[i].position = p.first;
    node_geometry[i].prev_position = p.second;
    // mbr.growToContain(p.first.x, p.first.y);
    version++;
  }
#if 0
  void setNodeColor2(int i, const graph_color_s & c) {
    node_geometry[i].color = c;
    version++;
  }
  void setNodeColor2(int i, const canvas::Color & c);
#endif
  
  void setNodeTexture(int i, int texture) {
    node_geometry[i].texture = texture;
    version++;
  }

  void setLabelTexture(const skey & key, int texture);  
  void setLabelTexture(int i, int texture) {
    node_geometry[i].label_texture = texture;
    version++;
  }
  
  void clearTextures(int clear_flags) {
    for (auto & nd : node_geometry) {
      if (clear_flags & CLEAR_LABELS) {
	nd.label_texture = 0;
      }
      if (clear_flags & CLEAR_NODES) {
	nd.texture = DEFAULT_PROFILE;
      }
    }
  }

  void clear() {
    node_geometry.clear();
    node_cache.clear();
    nodes.clear();
  }

  const node_data_s & getNodeData(int i) const { return node_geometry[i]; }
  node_data_s & getNodeData(int i) { return node_geometry[i]; }

  void setLabel(int i, const std::string & text) {
    auto & nd = node_geometry[i];
    if (nd.label != text) {
      nd.label = text;
      nd.label_texture = 0;
    }
  }

  std::vector<int> addNodes(size_t n) {
    std::vector<int> v;
    while (n--) {
      v.push_back(add());
    }
    return v;
  }

#if 0
  void setNodeColorByColumn(int column);
#endif

  const std::pair<glm::vec3, glm::vec3> getPositions(int i) const {
    return std::pair<glm::vec3, glm::vec3>(node_geometry[i].position, node_geometry[i].prev_position);
  }
  
  std::vector<node_data_s> & getGeometry() { return node_geometry; }
  const std::vector<node_data_s> & getGeometry() const { return node_geometry; }
  std::vector<node_position_data_s> copyPositions() const {
    std::vector<node_position_data_s> v;
    v.resize(node_geometry.size());
    for (unsigned int i = 0, n = node_geometry.size(); i < n; i++) {
      v[i].position = node_geometry[i].position;
      v[i].prev_position = node_geometry[i].prev_position;
    }
    return v;
  }
  void storePositions(const std::vector<node_position_data_s> & v) {
    for (unsigned int i = 0, n = v.size(); i < n; i++) {
      node_geometry[i].position = v[i].position;
      node_geometry[i].prev_position = v[i].prev_position;
    }
    version++;
  }
  
#if 0
  NodeIterator begin_nodes() { return NodeIterator(&(node_geometry.front())); }
  NodeIterator end_nodes() { return NodeIterator(&(node_geometry.back())) + 1; }
#endif  
  
  void setLabelStyle(LabelStyle style) { label_style = style; }
  LabelStyle getLabelStyle() const { return label_style; }
  
  void setAlpha3(float f) { alpha = f; }
  void updateAlpha() { alpha *= 0.99f; }
  float getAlpha2() const { return alpha; }
  float isRunning() const { return alpha >= 0.0005f; }

  void resume2();
  void stop() { alpha = 0.0f; }

  int getVersion() const { return version; }

  int getSRID() const { return srid; }
  void setSRID(int _srid) { srid = _srid; }

  int getZeroDegreeGroup() const { return zerodegree_node_id; }
  int createZeroDegreeGroup() {
    if (zerodegree_node_id == -1) {
      zerodegree_node_id = add();
      setRandomPosition(zerodegree_node_id);
    }
    return zerodegree_node_id;
  }
  int getPairsGroup() const { return pairs_node_id; }
  int createPairsGroup() {
    if (pairs_node_id == -1) {
      pairs_node_id = add();
      setRandomPosition(pairs_node_id);
    }
    return pairs_node_id;
  }
  int getOneDegreeNode(int node_id) {
    auto & nd = node_geometry[node_id];
    if (nd.group_node != -1) return nd.group_node;
    int group_node = add(); // nd is no longer valid after add
    node_geometry[node_id].group_node = group_node;
    setRandomPosition(group_node);
    return group_node;
  }

  int addArcGeometry(const ArcData2D & data) {
    int arc_id = 1 + int(arc_geometry.size());
    arc_geometry.push_back(data);
    return arc_id;
  }
  const std::vector<ArcData2D> & getArcGeometry() const { return arc_geometry; }

  void setRandomPosition(int node_id);

  bool isTemporal() const { return testFlags(GF_TEMPORAL_GRAPH); }
  void setTemporal(bool t) { updateFlags(GF_TEMPORAL_GRAPH, t); } 
  
  bool isDynamic() const { return testFlags(GF_DYNAMIC); }
  void setDynamic(bool t) { updateFlags(GF_DYNAMIC, t); }

  bool hasSpatialData() const { return testFlags(GF_HAS_SPATIAL_DATA); }
  void setHasSpatialData(bool t) { updateFlags(GF_HAS_SPATIAL_DATA, t); }

  bool hasArcData() const { return testFlags(GF_HAS_ARC_DATA); }
  void setHasArcData(bool t) { updateFlags(GF_HAS_ARC_DATA, t); }

  bool hasTextures() const { return testFlags(GF_HAS_TEXTURES); }
  void setHasTextures(bool t) { updateFlags(GF_HAS_TEXTURES, t); }

  bool hasTemporalCoverage() const { return testFlags(GF_TEMPORAL_COVERAGE); }
  void setHasTemporalCoverage(bool t) { updateFlags(GF_TEMPORAL_COVERAGE, t); } 

  bool doFlattenHierarchy() const { return testFlags(GF_FLATTEN_HIERARCHY); }
  void setFlattenHierarchy(bool t) { updateFlags(GF_FLATTEN_HIERARCHY, t); }

  bool perNodeColorsEnabled() const { return testFlags(GF_PER_NODE_COLORS); }
  void setPerNodeColors(bool t) { updateFlags(GF_PER_NODE_COLORS, t); }

 protected:
  bool testFlags(unsigned int bit) const { return (flags & bit) != 0; }
  void updateFlags(unsigned int bit, bool t) { flags = (t ? flags | bit : flags & ~bit); }

  std::vector<node_data_s> node_geometry;
    
 private:
  std::map<skey, int> node_cache;
  table::Table nodes;
  SizeMethod size_method;
  LabelMethod label_method;
  LabelStyle label_style = LABEL_PLAIN;
  float alpha = 0.0f;
  int srid = 0, version = 1;
  int zerodegree_node_id = -1, pairs_node_id = -1;
  std::vector<ArcData2D> arc_geometry;
  std::string node_color_column;
  unsigned int flags = 0;
  Personality personality = NONE;
};

#endif
