#include "NodeArray.h"

#include <glm/gtc/packing.hpp>
#include <Color.h>
#include "../../personal/system/StringUtils.h"

#include <algorithm>

#define EPSILON 0.0000000001

using namespace std;

void
NodeArray::updateNodeAppearanceAll() {
  for (unsigned int i = 0; i < size(); i++) {
    updateNodeAppearanceSlow(i);
  }
}

void
NodeArray::updateNodeAppearanceSlow(int node_id) {
  string label, id, uname, name;
  int r = -1, g = -1, b = -1;

  for (auto & cd : getTable().getColumns()) {
    string n = StringUtils::toLower(cd.first);
    if (n == "r") {
      r = cd.second->getInt(node_id);
    } else if (n == "g") {
      g = cd.second->getInt(node_id);
    } else if (n == "b") {
      b = cd.second->getInt(node_id);
    } else if (label_method.getValue() == LabelMethod::AUTOMATIC_LABEL) {
      if (n == "label") {
	label = cd.second->getText(node_id);
      } else if (n == "uname") {
	uname = cd.second->getText(node_id);
      } else if (n == "name") {
	name = cd.second->getText(node_id);
      } else if (n == "id") {
	id = cd.second->getText(node_id);
      }
    }
  }
  if (label_method.getValue() != LabelMethod::FIXED_LABEL) {
    if (label_method.getValue() == LabelMethod::LABEL_FROM_COLUMN) {
      label = getTable()[label_method.getColumn()].getText(node_id);
    } else if (label_method.getValue() == LabelMethod::AUTOMATIC_LABEL && !label.empty()) {
      if (!uname.empty()) {
	label = uname;
      } else if (!name.empty()) {
	label = name;
      } else if (!id.empty()) {
	label = id;
      }    
    }
    setNodeLabel(node_id, label);
  }
  if (r >= 0 && g >= 0 && b >= 0) {
    canvas::Color c(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
    setNodeColor2(node_id, c);
  }
}

static table::Column * sort_col = 0;

static bool compareRows(const int & a, const int & b) {
  return sort_col->compare(a, b);
}

void
NodeArray::setNodeColorByColumn(int column) {
  sort_col = &(nodes[column]);
  if (size() <= 1) return;
  cerr << "setting colors by column " << sort_col->name() << endl;
  vector<int> v;
  for (int i = 0; i < size(); i++) {
      v.push_back(i);
  }
  sort(v.begin(), v.end(), compareRows);
  glm::vec3 c1(1.0, 0.0, 0.0), c2(1.0, 1.0, 0.0);
  for (int i = 0; i < v.size(); i++) {
    cerr << "node " << i << ": " << sort_col->getDouble(v[i]) << endl;
#if 1
    glm::vec3 c = glm::normalize(glm::mix(c1, c2, (float)(v.size() - 1 - i) / (v.size() - 1)));
#else
    Colorf c;
    c.setHSL(2.0f * (v.size() - 1 - i) / (v.size() - 1) / 3.0f, 0.75, 0.5);
#endif
    graph_color_s tmp = {
	(unsigned char)glm::clamp(int(c.r * 255.0f), 0, 255),
	  (unsigned char)glm::clamp(int(c.g * 255.0f), 0, 255),
	  (unsigned char)glm::clamp(int(c.b * 255.0f), 0, 255),
	  255
    };
    setNodeColor2(v[i], tmp);
  }
  version++;
  // updateFlags(GF_PER_NODE_COLORS, true);
}

void
NodeArray::setNormal(int i, const glm::vec4 & v) {
  node_geometry[i].normal = glm::packSnorm3x10_1x2(v);
  version++;
}

void
NodeArray::setNodeColor2(int i, const canvas::Color & c) {
  int r = int(c.red * 0xff), g = int(c.green * 0xff), b = int(c.blue * 0xff), a = int(c.alpha * 0xff);
  if (r > 255) r = 255;
  else if (r < 0) r = 0;
  if (g > 255) g = 255;
  else if (g < 0) g = 0;
  if (b > 255) b = 255;
  else if (b < 0) b = 0;
  if (a > 255) a = 255;
  else if (a < 0) a = 0;
  graph_color_s tmp = { (unsigned char)r, (unsigned char)g, (unsigned char)b, (unsigned char)a };
  setNodeColor2(i, tmp);
}
