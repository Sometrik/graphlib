#ifndef _NODEARRAY_H_
#define _NODEARRAY_H_

#include "NodeType.h"
#include "SizeMethod.h"
#include "../system/Mutex.h"
#include "skey.h"
#include "graph_color.h"
#include "LabelStyle.h"
#include "RenderMode.h"
#include "LabelMethod.h"

#define INITIAL_ALPHA		0.075f

#include <Table.h>
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

namespace canvas {
  class Color;
};

struct node_data_s {
  graph_color_s color; // 0
  glm::uint32 normal; // 4
  glm::vec3 position; // 8
  glm::vec3 prev_position; // 20
  float age, size; // 32
  short texture, flags; // 40
  NodeType type;
  int cluster_id;
  short label_texture;
  unsigned short label_visibility_val;
  std::string label;
  std::shared_ptr<Graph> nested_graph;
};

class NodeArray {
 public:
  friend class Graph;
  NodeArray() { }

  size_t size() const { return nodes.size(); }
  bool empty() const { return nodes.empty(); }
  
  table::Table & getTable() { return nodes; }
  const table::Table & getTable() const { return nodes; }

  void setNodeSizeMethod(SizeMethod m) { node_size_method = m; }
  const SizeMethod & getNodeSizeMethod() const { return node_size_method; }

  void setLabelMethod(const LabelMethod & m) { label_method = m; }
  const LabelMethod & getLabelMethod() const { return label_method; }

  void updateAppearance();
  void updateNodeAppearanceAll();
  void updateNodeAppearanceSlow(int node_id);

  // std::string getNodeName(int i) const { return nodes["name"].getText(i); }
  // std::string getNodeUsername(int i) const { return nodes["uname"].getText(i); }

  std::map<skey, int> & getNodeCache() { return node_cache; } 
  const std::map<skey, int> & getNodeCache() const { return node_cache; } 

  int addNode(NodeType type = NODE_ANY, float size = 0.0f, float age = 0.0f) {
    int node_id = node_geometry.size();
    node_geometry.push_back({ { 200, 200, 200, 255 }, 0, glm::vec3(), glm::vec3(), age, size, 0, NODE_SELECTED, type, -1, 0, 0, "", std::shared_ptr<Graph>() });
    version++;
    while (nodes.size() < node_geometry.size()) {
      nodes.addRow();
    }
    // updateNodeSize(node_id);
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
  void setNodeColor2(int i, const graph_color_s & c) {
    node_geometry[i].color = c;
    version++;
  }
  void setNodeColor2(int i, const canvas::Color & c);

  void setNodeTexture(int i, int texture) {
    node_geometry[i].texture = texture;
    version++;
  }

  void setLabelTexture(int i, int texture) {
    node_geometry[i].label_texture = texture;
    version++;
  }

  void setNodeLabelVisibilityValue(int i, float f) {
    int f2 = int(f * 65535);
    if (f2 < 0) f2 = 0;
    else if (f2 > 65535) f2 = 65535;
    node_geometry[i].label_visibility_val = (unsigned short)f2;
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
  
  void clearTextures(int clear_flags = CLEAR_ALL) {
    for (int i = 0; i < size(); i++) {
      if (clear_flags & CLEAR_LABELS) {
	node_geometry[i].label_texture = 0;
	node_geometry[i].label_visibility_val = 0;
	node_geometry[i].flags &= ~NODE_LABEL_VISIBLE;
      }
      if (clear_flags & CLEAR_NODES) {
	node_geometry[i].texture = DEFAULT_PROFILE;
      }
    }
  }

  void clear() {
    node_geometry.clear();
    node_cache.clear();
    nodes.clear();
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
  
  const graph_color_s & getNodeColor(int i) const { return node_geometry[i].color; }
  bool getNodeLabelVisibility(int i) const { return node_geometry[i].flags & NODE_LABEL_VISIBLE ? true : false; }
  float getNodeLabelVisibilityValue(int i) const { return node_geometry[i].label_visibility_val / 65535.0f; }
  
  void setNodeLabel(int i, const std::string & text) {
    if (node_geometry[i].label != text) {
      node_geometry[i].label = text;
      node_geometry[i].label_texture = 0;
    }
  }
  
  void setNestedGraph(int i, std::shared_ptr<Graph> graph) {
    node_geometry[i].nested_graph = graph;
  }

  std::vector<int> addNodes(size_t n) {
    std::vector<int> v;
    while (n--) {
      v.push_back(addNode());
    }
    return v;
  }

  void setNodeColorByColumn(int column);

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

#if 0
  NodeIterator begin_nodes() { return NodeIterator(&(node_geometry.front())); }
  NodeIterator end_nodes() { return NodeIterator(&(node_geometry.back())) + 1; }
#endif
  
  void setDefaultSymbolId(int symbol_id) { default_symbol_id = symbol_id; }
  int getDefaultSymbolId() const { return default_symbol_id; }

  std::vector<node_data_s> node_geometry;

  const std::string & getNodeLabel(int i) const { return node_geometry[i].label; }
  // void clearLabelTexture(int i) { node_geometry[i].label_texture = 0; }

  void setLabelStyle(LabelStyle style) { label_style = style; }
  LabelStyle getLabelStyle() const { return label_style; }

  void applyGravity(float gravity);
  void applyDragAndAge(RenderMode mode, float friction);
  
  void setAlpha3(float f) { alpha = f; }
  void updateAlpha() { alpha *= 0.99f; }
  float getAlpha2() const { return alpha; }
  void resume2() { alpha = INITIAL_ALPHA; }
  void stop() { alpha = 0.0f; }

  int getVersion() const { return version; }
  
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
  SizeMethod node_size_method;
  LabelMethod label_method;
  LabelStyle label_style = LABEL_PLAIN;
  int default_symbol_id = 0;
  float alpha = 0.0f;
  int version = 1;

  mutable int num_readers = 0;
  mutable Mutex mutex, writer_mutex;
};

#endif
