#include "NodeArray.h"

#include <glm/gtc/packing.hpp>

#include <algorithm>
#include <cassert>
#include <sstream>

using namespace std;

NodeArray::NodeArray() : size_method(SizeMethod::CONSTANT, 1.0f) {
}

std::string
NodeArray::getNodeLabel(int node_id) const {
  string label, uname, name;
  long long id = 0;

  auto & nd = getNodeData(node_id);
  NodeType type = nd.type;

  assert(type != NODE_ANY_MALE && type != NODE_ANY_FEMALE && type != NODE_ANY_PAGE &&
	 type != NODE_TOKEN && type != NODE_USER);
  
  if (type == NODE_COMMUNITY) return "";
  else if (type == NODE_LANG_ATTRIBUTE) return "(language)";
  else if (type == NODE_ATTRIBUTE) return "(attr)";

  if (label_method.getValue() == LabelMethod::AUTOMATIC_LABEL) {
    for (auto & cd : getTable().getColumns()) {
      auto & n = cd.first;
      if (strcasecmp(n.c_str(), "label") == 0) {
	label = cd.second->getText(node_id);
      } else if (strcasecmp(n.c_str(), "uname") == 0) {
	uname = cd.second->getText(node_id);
      } else if (strcasecmp(n.c_str(), "name") == 0) {
	name = cd.second->getText(node_id);
      } else if (strcasecmp(n.c_str(), "id") == 0) {
	id = cd.second->getInt64(node_id);
      }
    }
  }
  if (label_method.getValue() == LabelMethod::LABEL_FROM_COLUMN) {
    label = getTable()[label_method.getColumn()].getText(node_id);
  } else if (label_method.getValue() == LabelMethod::AUTOMATIC_LABEL && label.empty()) {
    int source_id = getTable()["source"].getInt(node_id);
    bool is_vimeo = source_id == 13 || source_id == 18 || source_id == 69;
    if (!uname.empty() && type != NODE_URL && type != NODE_IMAGE && !is_vimeo) {
      label = uname;
    } else if (!name.empty()) {
      label = name;
    } else if (!uname.empty()) {
      label = uname;
    } else if (id) {
      label = to_string(id);
    }
  }
  return label;
}

static table::Column * sort_col = 0;

static bool compareRows(const int & a, const int & b) {
  return sort_col->compare(a, b);
}

void
NodeArray::setLabelTexture(const skey & key, int texture) {
  auto it2 = getNodeCache().find(key);
  if (it2 != getNodeCache().end()) {
    setLabelTexture(it2->second, texture);
  }
}

void
NodeArray::setRandomPosition(int node_id, bool use_2d) {
  glm::vec3 v1( 256.0f * rand() / RAND_MAX - 128.0f,
		256.0f * rand() / RAND_MAX - 128.0f,
		use_2d ? 0.0f : 256.0f * rand() / RAND_MAX - 128.0f
		);
  setPosition(node_id, v1);
}

int
NodeArray::createNode2D(double x, double y) {
  ostringstream key;
  key << x << "/" << y;
  map<string, int>::iterator it = node_position_cache.find(key.str());
  if (it != node_position_cache.end()) {
    return it->second;
  } else {
    int node_id = node_position_cache[key.str()] = add();
    setPosition(node_id, glm::vec3(x, y, 0.0f));
    return node_id;  
  }
}

int
NodeArray::createNode3D(double x, double y, double z) {
  ostringstream key;
  key << x << "/" << y << "/" << z;
  map<string, int>::iterator it = node_position_cache.find(key.str());
  if (it != node_position_cache.end()) {
    return it->second;
  } else {
    int node_id = node_position_cache[key.str()] = add();
    setPosition(node_id, glm::vec3(x, y, z));
    return node_id;  
  }
}

bool
NodeArray::hasNode(double x, double y, int * r) const {
  ostringstream key;
  key << x << "/" << y;
  auto it = node_position_cache.find(key.str());
  if (it != node_position_cache.end()) {
    if (r) *r = it->second;
    return true;
  } else {
    return false;
  }
}

pair<int, int>
NodeArray::createNodesForArc(const ArcData2D & arc, bool rev) {
  assert(arc.data.size() >= 2);
  auto & v1 = arc.data.front();
  auto & v2 = arc.data.back();
  int node1 = createNode2D(v1.x, v1.y);
  int node2 = createNode2D(v2.x, v2.y);
  assert(node1 >= 0 && node2 >= 0);
  if (rev) {
    return pair<int, int>(node1, node2);
  } else {
    return pair<int, int>(node2, node1);
  }
}
