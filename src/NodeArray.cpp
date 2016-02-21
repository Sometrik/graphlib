#include "NodeArray.h"

#include <glm/gtc/packing.hpp>
#include <Color.h>
#include "../../personal/system/StringUtils.h"

#include <algorithm>

#define EPSILON 0.0000000001
#define INITIAL_ALPHA		0.075f

using namespace std;

NodeArray::NodeArray()
  : size_method(SizeMethod::CONSTANT, 1.0f),
    node_color(0.0f, 0.0f, 0.0f, 0.0f),
    edge_color(0.0f, 0.0f, 0.0f, 0.0f),
    face_color(0.0f, 0.0f, 0.0f, 0.0f)
{
  
}

void
NodeArray::updateAppearance() {
  for (unsigned int i = 0; i < size(); i++) {
    updateAppearanceSlow(i);
  }
}

void
NodeArray::updateAppearanceSlow(int node_id) {
  string label, id, uname, name;

  if (label_method.getValue() == LabelMethod::AUTOMATIC_LABEL) {
    for (auto & cd : getTable().getColumns()) {
      string n = StringUtils::toLower(cd.first);
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
    } else if (label_method.getValue() == LabelMethod::AUTOMATIC_LABEL && label.empty()) {
      if (!uname.empty()) {
	label = uname;
      } else if (!name.empty()) {
	label = name;
      } else if (!id.empty()) {
	label = id;
      }    
    }
    // cerr << "setting label for node " << node_id << ": " << label << endl;
    setLabel(node_id, label);
  }
}

static table::Column * sort_col = 0;

static bool compareRows(const int & a, const int & b) {
  return sort_col->compare(a, b);
}

#if 0
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
#endif

void
NodeArray::setNormal(int i, const glm::vec4 & v) {
  node_geometry[i].normal = glm::packSnorm3x10_1x2(v);
  version++;
}

#if 0
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
#endif

void
NodeArray::resume2() {
  alpha = INITIAL_ALPHA;
}

void
NodeArray::setLabelTexture(const skey & key, int texture) {
  auto it2 = getNodeCache().find(key);
  if (it2 != getNodeCache().end()) {
    setLabelTexture(it2->second, texture);
  }
}
