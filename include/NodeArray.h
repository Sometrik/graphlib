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
#include <map>
#include <memory>

#define GF_TEMPORAL_GRAPH	1
#define GF_HAS_SPATIAL_DATA	4
#define GF_HAS_ARC_DATA		8
#define GF_DYNAMIC		16
#define GF_HAS_TEXTURES		32
#define GF_PER_NODE_COLORS	64
#define GF_FLATTEN_HIERARCHY	128

#define DEFAULT_PROFILE	0
#define PENDING_PROFILE	1
#define BOT_PROFILE	2
#define MALE_NODE	3
#define FEMALE_NODE	4
#define GROUP_NODE	5
#define DOCUMENT_NODE	6
#define IMAGE_NODE	7
#define FLAG_NODE	8

#define CLEAR_LABELS	1
#define CLEAR_NODES	2
#define CLEAR_ALL	(CLEAR_LABELS | CLEAR_NODES)

struct node_position_data_s {
  glm::vec3 position, prev_position;
};

struct node_data_s {
  glm::vec3 position;
  short texture, label_texture;
  NodeType type;
  float alpha;
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

  std::string getNodeLabel(int node_id) const;

  std::map<skey, int> & getNodeCache() { return node_cache; } 
  const std::map<skey, int> & getNodeCache() const { return node_cache; } 

  int add(NodeType type = NODE_ANY) {
    int node_id = node_geometry.size();
    node_geometry.push_back({ glm::vec3(), 0, 0, type, 0.0f });
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
    node_geometry[i].position = v;
    version++;
  }
  void setPosition(int i, const glm::vec2 & v) {
    setPosition(i, glm::vec3(v, 0.0f));
  }
  
  void setNodeTexture(int i, int texture) {
    node_geometry[i].texture = texture;
    version++;
  }

  void setLabelTexture(const skey & key, int texture);  
  void setLabelTexture(int i, int texture) {
    node_geometry[i].label_texture = texture; // don't increment version for labels
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

  std::vector<int> addNodes(size_t n) {
    std::vector<int> v;
    while (n--) {
      v.push_back(add());
    }
    return v;
  }
  
  std::vector<node_data_s> & getGeometry() { return node_geometry; }
  const std::vector<node_data_s> & getGeometry() const { return node_geometry; }
  void updatePositions(std::vector<node_position_data_s> & v) const {
    unsigned int old_position = v.size();
    v.resize(node_geometry.size());
    for (unsigned int i = old_position, n = node_geometry.size(); i < n; i++) {
      v[i].position = node_geometry[i].position;
      v[i].prev_position = node_geometry[i].position;
    }
  }  
  void storePositions(const std::vector<node_position_data_s> & v) {
    unsigned int n = v.size() < node_geometry.size() ? v.size() : node_geometry.size();
    for (unsigned int i = 0; i < n; i++) {
      node_geometry[i].position = v[i].position;
    }
    version++;
  }
  
#if 0
  NodeIterator begin_nodes() { return NodeIterator(&(node_geometry.front())); }
  NodeIterator end_nodes() { return NodeIterator(&(node_geometry.back())) + 1; }
#endif  
  
  void setLabelStyle(LabelStyle style) { label_style = style; }
  LabelStyle getLabelStyle() const { return label_style; }
  
  void setTopLevelAlpha(float f) { alpha = f; }
  float getTopLevelAlpha() const { return alpha; }
  void updateTopLevelAlpha() { alpha *= 0.990f; }

  int getVersion() const { return version; }

  int getSRID() const { return srid; }
  void setSRID(int _srid) { srid = _srid; }

  int addArcGeometry(const ArcData2D & data) {
    int arc_id = 1 + int(arc_geometry.size());
    arc_geometry.push_back(data);
    return arc_id;
  }
  const std::vector<ArcData2D> & getArcGeometry() const { return arc_geometry; }

  void setRandomPosition(int node_id, bool use_2d = true);

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

  bool doFlattenHierarchy() const { return testFlags(GF_FLATTEN_HIERARCHY); }
  void setFlattenHierarchy(bool t) { updateFlags(GF_FLATTEN_HIERARCHY, t); }

  bool perNodeColorsEnabled() const { return testFlags(GF_PER_NODE_COLORS); }
  void setPerNodeColors(bool t) { updateFlags(GF_PER_NODE_COLORS, t); }

  int getCommunityById(int id) const {
    auto it = communities.find(id);
    if (it != communities.end()) return it->second;
    return -1;
  }

  int createCommunity(int id) {
    int community_id = getCommunityById(id);
    if (community_id != -1) return community_id;
    communities[id] = community_id = add(NODE_COMMUNITY);
    setNodeTexture(community_id, GROUP_NODE);
    setRandomPosition(community_id);
    return community_id;
  }

  int getLanguageById(short id) const {
    auto it = languages.find(id);
    if (it != languages.end()) return it->second;
    return -1;
  }

  int createLanguage(short id) {
    int community_id = getLanguageById(id);
    if (community_id != -1) return community_id;
    languages[id] = community_id = add(NODE_LANG_ATTRIBUTE);
    setNodeTexture(community_id, FLAG_NODE);
    setRandomPosition(community_id);
    return community_id;
  }

  int getApplicationById(short id) const {
    auto it = applications.find(id);
    if (it != applications.end()) return it->second;
    return -1;
  }

  int createApplication(short id) {
    int community_id = getApplicationById(id);
    if (community_id != -1) return community_id;
    applications[id] = community_id = add(NODE_ATTRIBUTE);
    setRandomPosition(community_id);
    return community_id;
  }

  int getMaleNode() const { return male_node_id; }
  int getFemaleNode() const { return female_node_id; }

  int createMaleNode() {
    if (male_node_id == -1) {
      male_node_id = add(NODE_ATTRIBUTE);
      setNodeTexture(male_node_id, MALE_NODE);
      setRandomPosition(male_node_id);
    }
    return male_node_id;
  }

  int createFemaleNode() {
    if (female_node_id == -1) {
      female_node_id = add(NODE_ATTRIBUTE);
      setNodeTexture(female_node_id, FEMALE_NODE);
      setRandomPosition(female_node_id);
    }
    return female_node_id;
  }

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
  int male_node_id = -1, female_node_id = -1;
  std::vector<ArcData2D> arc_geometry;
  std::string node_color_column;
  std::unordered_map<int, int> communities;
  std::unordered_map<short, int> languages;
  std::unordered_map<short, int> applications;
  unsigned int flags = 0;
  Personality personality = NONE;
};

#endif
