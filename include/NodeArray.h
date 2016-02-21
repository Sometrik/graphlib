#ifndef _NODEARRAY_H_
#define _NODEARRAY_H_

#include <skey.h>

#include "NodeType.h"
#include "SizeMethod.h"
#include "LabelStyle.h"
#include "RenderMode.h"
#include "LabelMethod.h"
#include "ArcData2D.h"
#include "Table.h"

#include "../system/Mutex.h"

#include <glm/glm.hpp>
#include <string>

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

struct node_data_s {
  glm::uint32 normal;
  glm::vec3 position;
  glm::vec3 prev_position;
  float age;
  short texture, flags;
  NodeType type;
  short label_texture;
  unsigned short label_visibility_val;  
  std::string label;
  int group_node;
  std::shared_ptr<Graph> nested_graph;
  
  bool getLabelVisibility() const { return flags & NODE_LABEL_VISIBLE ? true : false; }
  float getLabelVisibilityValue() const { return label_visibility_val / 65535.0f; }

  void setLabelVisibilityValue(float f) {
    int f2 = int(f * 65535);
    if (f2 < 0) f2 = 0;
    else if (f2 > 65535) f2 = 65535;
    label_visibility_val = (unsigned short)f2;
  }

  bool setLabelVisibility(bool t) {
    bool orig_t = flags | NODE_LABEL_VISIBLE ? true : false;
    if (t != orig_t || 1) {
      if (t) {
	flags |= NODE_LABEL_VISIBLE;
      } else {
	flags &= ~NODE_LABEL_VISIBLE;
      }
      return true;
    } else {
      return false;
    }
  }
};

class NodeArray {
 public:
  friend class Graph;
  NodeArray();

  size_t size() const { return nodes.size(); }
  bool empty() const { return nodes.empty(); }
  
  table::Table & getTable() { return nodes; }
  const table::Table & getTable() const { return nodes; }

  void setNodeSizeMethod(const SizeMethod & m) { size_method = m; }
  const SizeMethod & getNodeSizeMethod() const { return size_method; }

  void setLabelMethod(const LabelMethod & m) { label_method = m; }
  const LabelMethod & getLabelMethod() const { return label_method; }

  void updateAppearance();
  void updateAppearanceSlow(int node_id);

  std::map<skey, int> & getNodeCache() { return node_cache; } 
  const std::map<skey, int> & getNodeCache() const { return node_cache; } 

  int add(NodeType type = NODE_ANY, float age = 0.0f) {
    int node_id = node_geometry.size();
    node_geometry.push_back({ 0, glm::vec3(), glm::vec3(), age, 0, NODE_SELECTED, type, 0, 0, "", -1, std::shared_ptr<Graph>() });
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
  void updatePrevPosition(int i, const glm::vec3 & v) {
    node_geometry[i].prev_position = v;    
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
  void setNormal(int i, const glm::vec4 & v);
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
	nd.label_visibility_val = 0;
	nd.flags &= ~NODE_LABEL_VISIBLE;
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

  bool updateLabelValues(int i, float visibility) {
    auto & nd = getNodeData(i);
    float vv = nd.getLabelVisibilityValue() + visibility;
    if (vv < 0) vv = 0;
    else if (vv > 1) vv = 1;
    nd.setLabelVisibilityValue(vv);
    if (vv >= 0.75) return nd.setLabelVisibility(true);
    else if (vv <= 0.25) return nd.setLabelVisibility(false);
    else return false;
  }

  void setLabel(int i, const std::string & text) {
    auto & nd = node_geometry[i];
    if (nd.label != text) {
      nd.label = text;
      nd.label_texture = 0;
    }
  }

  void setNestedGraph(int i, std::shared_ptr<Graph> graph) {
    node_geometry[i].nested_graph = graph;
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

  const glm::vec3 & getPosition(int i) const {
    return node_geometry[i].position;
  }
  const glm::vec3 & getPrevPosition(int i) const {
    return node_geometry[i].prev_position;
  }
  const std::pair<glm::vec3, glm::vec3> getPositions(int i) const {
    return std::pair<glm::vec3, glm::vec3>(node_geometry[i].position, node_geometry[i].prev_position);
  }
  
  std::vector<node_data_s> & getGeometry() { return node_geometry; }

#if 0
  NodeIterator begin_nodes() { return NodeIterator(&(node_geometry.front())); }
  NodeIterator end_nodes() { return NodeIterator(&(node_geometry.back())) + 1; }
#endif
  
  std::vector<node_data_s> node_geometry;

  void setLabelStyle(LabelStyle style) { label_style = style; }
  LabelStyle getLabelStyle() const { return label_style; }
  
  const glm::vec4 & getDefaultNodeColor() const { return node_color; }
  const glm::vec4 & getDefaultEdgeColor() const { return edge_color; }
  const glm::vec4 & getDefaultFaceColor() const { return face_color; }
  
  void setDefaultNodeColor(const glm::vec4 & color) { node_color = color; }
  void setDefaultEdgeColor(const glm::vec4 & color) { edge_color = color; }
  void setDefaultFaceColor(const glm::vec4 & color) { face_color = color; }

  void setNodeVisibility(bool t) { show_nodes = t; }
  void setEdgeVisibility(bool t) { show_edges = t; }
  void setFaceVisibility(bool t) { show_faces = t; }
  void setLabelVisibility(bool t) { show_labels = t; }

  bool getNodeVisibility() const { return show_nodes; }
  bool getEdgeVisibility() const { return show_edges; }
  bool getFaceVisibility() const { return show_faces; }
  bool getLabelVisibility() const { return show_labels; }

  void setAlpha3(float f) { alpha = f; }
  void updateAlpha() { alpha *= 0.99f; }
  float getAlpha2() const { return alpha; }
  void resume2();
  void stop() { alpha = 0.0f; }

  int getVersion() const { return version; }

  int getSRID() const { return srid; }
  void setSRID(int _srid) { srid = _srid; }

  bool hasZeroDegreeNode() const { return zerodegree_node_id != -1; }
  int getZeroDegreeNode() {
    if (zerodegree_node_id == -1) {
      zerodegree_node_id = add();
    }
    return zerodegree_node_id;
  }
  int getOneDegreeNode(int node_id) {
    auto & nd = node_geometry[node_id];
    if (nd.group_node == -1) {
      nd.group_node = add();
    }
    return nd.group_node;
  }

  int addArcGeometry(const ArcData2D & data) {
    int arc_id = 1 + int(arc_geometry.size());
    arc_geometry.push_back(data);
    return arc_id;
  }
  const std::vector<ArcData2D> & getArcGeometry() const { return arc_geometry; }

 protected:

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

  std::map<skey, int> node_cache;
  table::Table nodes;
  SizeMethod size_method;
  LabelMethod label_method;
  LabelStyle label_style = LABEL_PLAIN;
  float alpha = 0.0f;
  int srid = 0, version = 1;
  bool show_nodes = true, show_edges = true, show_faces = true, show_labels = true;
  glm::vec4 node_color, edge_color, face_color;
  int zerodegree_node_id = -1;
  std::vector<ArcData2D> arc_geometry;
  
  mutable int num_readers = 0;
  mutable Mutex mutex, writer_mutex;
};

#endif
