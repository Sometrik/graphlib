#include "Graph.h"

#include "../system/StringUtils.h"

#include "DisplayInfo.h"
#include "ColorProvider.h"
#include "Controller.h"
#include "RenderMode.h"
#include "ui/ArtProvider.h"
#include "ui/TextureAtlas.h"

#include <VBO.h>
#include <algorithm>
#include <iostream>
#include <typeinfo>

#ifndef _WIN32
#include "community/BinaryGraph.h"
#include "community/Community.h"
#endif

#include <glm/gtc/packing.hpp>

#define EPSILON 0.0000000001

using namespace std;
using namespace table;

int Graph::next_id = 1;

Graph::Graph(int _dimensions, int _id) : dimensions(_dimensions),
					 id(_id),
					 node_color(0.0f, 0.0f, 0.0f, 0.0f),
					 edge_color(0.0f, 0.0f, 0.0f, 0.0f),
					 region_color(0.0f, 0.0f, 0.0f, 0.0f)
{
  if (!id) id = next_id++;
}


Graph::~Graph() {
  
}

Graph::Graph(const Graph & other)
  : nodes(other.nodes),
    edges(other.edges),
    faces(other.faces),
    regions(other.regions),
    shells(other.shells),
    face_attributes(other.face_attributes),
    region_attributes(other.region_attributes),
    node_color_column(other.node_color_column),
    node_geometry(other.node_geometry),
    node_geometry2(other.node_geometry2),
    srid(other.srid),
    version(other.version),
    dimensions(other.dimensions),
    show_nodes(other.show_nodes),
    show_edges(other.show_edges),
    show_regions(other.show_regions),
    show_labels(other.show_labels),
    node_color(other.node_color),
    edge_color(other.edge_color),
    region_color(other.region_color),
    flags(other.flags),
    source_id(other.source_id),
    node_cache(other.node_cache),
    personality(other.personality),
    node_size_method(other.node_size_method),
    label_method(other.label_method)
{
  id = next_id++;
  for (auto & d : node_geometry2) {
    if (d.nested_graph.get()) {
      d.nested_graph = std::shared_ptr<Graph>(d.nested_graph->copy());
    }
  }
}
 
bool
Graph::hasEdge(int n1, int n2) const {
  int edge = getNodeFirstEdge(n1);
  while (edge != -1) {
    if (getEdgeTargetNode(edge) == n2) {
      return true;
    }
    edge = getNextNodeEdge(edge);
  }
  return false;
}

static Column * sort_col = 0;

static bool compareRows(const int & a, const int & b) {
  return sort_col->compare(a, b);
}

void
Graph::updateNodeAppearanceSlow(int node_id) {
  string label, id, uname, name;
  int r = -1, g = -1, b = -1;

  for (map<string, std::shared_ptr<Column> >::const_iterator it = getNodeData().getColumns().begin(); it != getNodeData().getColumns().end(); it++) {
    string n = StringUtils::toLower(it->first);
    if (n == "r") {
      r = it->second->getInt(node_id);
    } else if (n == "g") {
      g = it->second->getInt(node_id);
    } else if (n == "b") {
      b = it->second->getInt(node_id);
    } else if (label_method.getValue() == LabelMethod::AUTOMATIC_LABEL) {
      if (n == "label") {
	label = it->second->getText(node_id);
      } else if (n == "uname") {
	uname = it->second->getText(node_id);
      } else if (n == "name") {
	name = it->second->getText(node_id);
      } else if (n == "id") {
	id = it->second->getText(node_id);
      }
    }
  }
  if (label_method.getValue() != LabelMethod::FIXED_LABEL) {
    if (label_method.getValue() == LabelMethod::LABEL_FROM_COLUMN) {
      label = getNodeData()[label_method.getColumn()].getText(node_id);
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

void
Graph::updateAppearance() {
  if (node_size_method.getValue() == SizeMethod::SIZE_FROM_COLUMN) {
    Column & sc = nodes[node_size_method.getColumn()];
    for (unsigned int i = 0; i < getNodeCount(); i++) {
      node_geometry[i].size = 2 * (1 + log(1 + sc.getDouble(i)) / log(1.5));
    }
    version++;
  } else if (node_size_method.getValue() == SizeMethod::SIZE_FROM_NODE_COUNT) {
    for (unsigned int i = 0; i < getNodeCount(); i++) {
      auto & nested_graph = node_geometry2[i].nested_graph;
      float a = nested_graph.get() ? nested_graph->getNodeCount() : 0;
      node_geometry[i].size = 2 * (1 + log(1 + a) / log(2));
    }
    version++;
  }
}

void
Graph::setNodeColorByColumn(int column) {
  sort_col = &(nodes[column]);
  if (getNodeCount() <= 1) return;
  cerr << "setting colors by column " << sort_col->name() << endl;
  vector<int> v;
  for (int i = 0; i < getNodeCount(); i++) {
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
  updateFlags(GF_PER_NODE_COLORS, true);
}

void
Graph::setRegionColorByColumn(int column) {
  sort_col = &(regions[column]);
  cerr << "setting colors by column " << sort_col->name() << endl;
  vector<int> v;
  for (int i = 0; i < getRegionCount(); i++) {
      v.push_back(i);
  }
  sort(v.begin(), v.end(), compareRows);
  glm::vec3 c1(0.5f, 0.1f, 0.9f), c2(0.05f, 0.01f, 0.09f);
  for (int i = 0; i < v.size(); i++) {
    cerr << "region " << i << ": " << sort_col->getDouble(v[i]) << endl;
#if 1
    glm::vec3 c = glm::normalize(glm::mix(c1, c2, (float)(v.size() - 1 - i) / (v.size() - 1)));
#else
    Colorf c;
    c.setHSL(2.0f * (v.size() - 1 - i) / (v.size() - 1) / 3.0f, 0.75, 0.5);
#endif
    region_attributes[v[i]].color = { (unsigned char)glm::clamp(int(c.r * 255.0f), 0, 255),
				      (unsigned char)glm::clamp(int(c.g * 255.0f), 0, 255),
				      (unsigned char)glm::clamp(int(c.b * 255.0f), 0, 255),
				      255
    };

  }
  version++;
}

void
Graph::randomizeGeometry(bool use_2d) {
  assert(!hasSpatialData());
  mbr = Rect2d();
  int num_nodes = getNodeCount();
  for (unsigned int i = 0; i < num_nodes; i++) {
    glm::vec3 v( 100.0 * ((float)rand() / RAND_MAX) - 50,
		 100.0 * ((float)rand() / RAND_MAX) - 50,
		 (use_2d ? 0 : 100.0 * ((float)rand() / RAND_MAX) - 50)
		 );
    node_geometry[i].position = node_geometry[i].prev_position = v;
    mbr.growToContain(v.x, v.y);

    auto & graph = node_geometry2[i].nested_graph;
    if (graph.get() && !graph->hasSpatialData()) {
      graph->randomizeGeometry(use_2d);       
    }
  }
  for (auto & g : final_graphs) {
    g->randomizeGeometry(use_2d);
  }
}

void
Graph::createRegionVBO(VBO & vbo, bool spherical, float earth_radius) const {
  if (!getRegionCount()) {
    return;
  }
  
  vector<arc_data_2d_s> new_geometry;
  vector<unsigned int> front_indices, back_indices;

  glm::vec3 normal(0.0f);

  if (hasArcData()) {
    cerr << "creating complex region VBO\n"; 
    for (unsigned int ri = 0; ri < getRegionCount(); ri++) {   
      auto this_color = getRegionColor(ri);
      glm::vec4 color( this_color.r * 255.0f, this_color.g * 255.0f, this_color.b * 255.0f, this_color.a * 255.0f );
      if (!color.w || 1) color = getDefaultRegionColor();
      if (!color.w) color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
      
      int face = getRegionFirstFace(ri);
      while (face != -1) {
	assert(face >= 0 && face < getFaceCount());
	
	list<int> edges;
	int edge = getFaceFirstEdge(face);
	while (edge != -1) {
	  auto & ed = getEdgeAttributes(edge);
	  // pair<int, int> ed = getFaceEdgeSourceNodeAndDirection(face, edge);
	  // assert(ed.first != -1);
	  // edges.push_front(pair<int, int>(edge, ed.second));
	  // edges.push_back(edge);
	  assert(ed.arc);
	  if (ed.arc) edges.push_back(ed.arc);
	  edge = getNextFaceEdge(edge);
	  // assert(edge >= -1 && edge < (int)getEdgeCount());
	}
	if (!edges.empty()) {
	  int start_index = new_geometry.size();
	  int vi = 0;
	  for (auto & arc_id : edges) {
	    auto & geometry = arc_geometry[abs(arc_id) - 1].data;
	    int dir = arc_id > 0 ? 1 : -1;
	    assert(!geometry.empty());
	    for (int i = 0; i < geometry.size(); i++) {
	      auto & v = geometry[dir == -1 ? geometry.size() - 1 - i : i];
	      new_geometry.push_back({ (unsigned char)(color.x * 255), (unsigned char)(color.y * 255), (unsigned char)(color.z * 255), (unsigned char)(color.w * 255), glm::vec2((float)v.x, (float)v.y) });
	      
	      if (vi >= 2) {
		int i1 = start_index, i2 = start_index + vi, i3 = start_index + vi - 1;
		auto & A = new_geometry[i1].position, & B = new_geometry[i2].position, & C = new_geometry[i3].position;
		glm::vec3 AB(B - A, 0.0f), AC(C - A, 0.0f);
		glm::vec3 normal = glm::normalize(glm::cross(AB, AC));
		if (normal.z < 0.0f) {
		  front_indices.push_back(i1);
		  front_indices.push_back(i2);
		  front_indices.push_back(i3);
		} else {
		  back_indices.push_back(i1);
		  back_indices.push_back(i2);
		  back_indices.push_back(i3);
		}
	      }
	      vi++;
	    }
	  }
	}
	
	face = getRegionNextFace(ri, face);
      }
    }

    back_indices.insert(back_indices.begin(), front_indices.begin(), front_indices.end());
    
    cerr << "uploading, i = " << back_indices.size() << ", g = " << new_geometry.size() << endl;
    
    vbo.upload(VBO::ARCS_2D, &(new_geometry.front()), new_geometry.size() * sizeof(arc_data_2d_s));
    vbo.uploadIndices(&(back_indices.front()), back_indices.size() * sizeof(unsigned int));

    cerr << "done uploading\n";
    
  } else {
    return;
    assert(0);
  }
}

void
Graph::createEdgeVBO(VBO & vbo, bool is_spherical, float earth_radius) const {
  if (!getEdgeCount()) {
    return;
  }

  if (hasArcData()) {
    cerr << "loading arcs: this = " << typeid(*this).name() << endl;
    unsigned int num_vertices = 0, num_indices = 0;    
    for (unsigned int i = 0; i < getEdgeCount(); i++) {
      auto & ed = getEdgeAttributes(i);
      assert(ed.arc);
      if (ed.arc) {
	int j = abs(ed.arc) - 1;
	// cerr << "arc = " << ed.arc << ", size = " << arc_geometry.size() << endl;
	if (j < 0 || j >= arc_geometry.size()) {
	  cerr << "invalid arc, j = " << j << ", size = " << arc_geometry.size() << endl;
	  assert(0);
	}
	auto & arc = arc_geometry[j];
	num_vertices += arc.data.size();
	num_indices += (arc.data.size() - 1) * 2;
      }
    }
    cerr << "edges = " << getEdgeCount() << " edge vertices = " << num_vertices << ", indices = " << num_indices << endl;

    std::unique_ptr<arc_data_2d_s[]> new_geometry(new arc_data_2d_s[num_vertices]);
    std::unique_ptr<unsigned int[]> indices(new unsigned int[num_indices]);

#if 0
    glm::vec4 color(0.0f);
    if (!color.w || 1) color = getDefaultEdgeColor();
    if (!color.w) color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
#else
    glm::vec4 color = getDefaultEdgeColor();
#endif

    // premultiply
    color.x *= color.a;
    color.y *= color.a;
    color.z *= color.a;

    unsigned int vn = 0, in = 0;
    for (unsigned int i = 0; i < getEdgeCount(); i++) {
      auto & ed = getEdgeAttributes(i);
      assert(ed.arc);
      if (!ed.arc) continue;
      int arc_idx = abs(ed.arc) - 1;
      assert(arc_idx >= 0 && arc_idx < arc_geometry.size());
      auto & geometry = arc_geometry[arc_idx];
      int dir = ed.arc > 0 ? 1 : -1;
      for (unsigned int j = 0; j < geometry.size(); j++) {
	auto & v = geometry.data[dir == -1 ? geometry.size() - 1 - j : j];
	if (j != 0) {
	  indices[in++] = vn - 1;
	  indices[in++] = vn;
	}
	// cerr << "point = " << v.x << " " << v.y << " " << v.z << endl;
#if 1
	new_geometry[vn++] = {
	  (unsigned char)(color.x * 255), (unsigned char)(color.y * 255), (unsigned char)(color.z * 255), (unsigned char)(color.w * 255), glm::vec2((float)v.x, (float)v.y ) };
#else
	new_geometry.push_back({ color, normal, glm::vec2((float)v1.x, (float)v1.y ) });
#endif
      }
    }

    assert(in <= num_indices);
    assert(vn <= num_vertices);

    cerr << "uploading, vbo = " << &vbo << ", in = " << in << ", vn = " << vn << "\n";
    
    vbo.upload(VBO::ARCS_2D, new_geometry.get(), vn * sizeof(arc_data_2d_s));
    vbo.uploadIndices(indices.get(), in * sizeof(unsigned int));
  } else {
    unsigned int ec = getEdgeCount();
    unsigned int asize = 2 * ec * sizeof(line_data_s);
    std::unique_ptr<line_data_s[]> new_geometry(new line_data_s[2 * ec]);
    unsigned int vn = 0;
    auto end = end_edges();
    for (auto it = begin_edges(); it != end; ++it) {    
      auto & g1 = node_geometry[it->tail], & g2 = node_geometry[it->head];
      
      if (node_geometry2[it->tail].type == NODE_HASHTAG ||
	  node_geometry2[it->head].type == NODE_HASHTAG) continue;
      
      glm::vec3 pos1, pos2;
      if (is_spherical) {
	double lat1 = g1.position.y / 180.0 * M_PI, lon1 = g1.position.x / 180.0 * M_PI;
	double lat2 = g2.position.y / 180.0 * M_PI, lon2 = g2.position.x / 180.0 * M_PI;
	pos1 = glm::vec3(-earth_radius * cos(lat1) * cos(lon1),
			 earth_radius * sin(lat1),
			 earth_radius * cos(lat1) * sin(lon1)
			 );
	pos2 = glm::vec3(-earth_radius * cos(lat2) * cos(lon2),
			 earth_radius * sin(lat2),
			 earth_radius * cos(lat2) * sin(lon2)
			 );
      } else {
	pos1 = g1.position;
	pos2 = g2.position;
      }
      auto color1 = g1.color, color2 = g2.color;
      if ((g1.flags & NODE_SELECTED) || (g2.flags & NODE_SELECTED)) {
	color1 = color2 = { 100, 200, 255, 255 };
      }
      if (vn + 1 >= 2 * ec) {
	cerr << "graph data mangled\n";
	continue;
      }
      line_data_s s1 = { color1.r, color1.g, color1.b, g1.color.a, pos1, pos2, g1.age, g1.size };
      line_data_s s2 = { color2.r, color2.g, color2.b, g2.color.a, pos2, pos1, g2.age, g2.size };
#if 1
      *((line_data_s*)(new_geometry.get()) + vn) = s1;
      *((line_data_s*)(new_geometry.get()) + vn + 1) = s2;
      vn += 2;
#else
      new_geometry[vn++] = s1;
      new_geometry[vn++] = s2;
#endif
    }
    assert(vn <= 2 * ec);
    // cerr << "uploading edges: vn = " << vn << ", edges = " << ec << ", ptrs = " << new_geometry.get() << ", asize = " << asize << ", ssize = " << sizeof(line_data_s) << endl;
#if 1
    vbo.upload(VBO::EDGES, new_geometry.get(), vn * sizeof(line_data_s));
#endif
  }
}

void
Graph::createNodeVBOForSprites(VBO & vbo, bool is_spherical, float earth_radius) const {
  if (!getNodeCount()) {
    return;
  }

  if (!is_spherical) {
    assert(sizeof(node_data_s) == 11 * 4);

    vbo.upload(VBO::NODES, &(node_geometry.front()), getNodeCount() * sizeof(node_data_s));
  } else {
    vector<node_data_s> new_geometry;
    new_geometry.reserve(getNodeCount());
    for (auto & g : node_geometry) {
      double lat = g.position.y / 180.0 * M_PI, lon = g.position.x / 180.0 * M_PI;
      glm::vec3 pos(-earth_radius * cos(lat) * cos(lon),
		    earth_radius * sin(lat),
		    earth_radius * cos(lat) * sin(lon));
      new_geometry.push_back({
	  g.color,
	    g.normal,
	    pos, pos,
	    g.age, g.size, g.texture, g.flags });
    }
    vbo.upload(VBO::NODES, &(new_geometry.front()), new_geometry.size() * sizeof(node_data_s));
  }
}

void
Graph::createNodeVBOForQuads(VBO & vbo, const TextureAtlas & atlas, float node_scale, bool is_spherical, float earth_radius) const {
  if (!getNodeCount()) {
    return;
  }

  std::unique_ptr<vbo_data2_s[]> data(new vbo_data2_s[4 * getNodeCount()]);
  std::unique_ptr<unsigned int[]> indices(new unsigned int[6 * getNodeCount()]);
  
  vbo_data2_s * current_data = data.get();
  unsigned int * current_index = indices.get();

  glm::vec3 normal(1.0f, 0.0f, 0.0f);
  int idx = 0;

  for (int i = 0; i < getNodeCount(); i++) { 
    auto & pd = getNodeData(i);
    auto & sd = getNodeSecondaryData(i);

    int texture = pd.texture;
    float size = pd.size / node_scale;
    
    glm::vec3 v = pd.position;
    glm::vec4 color(pd.color.r / 255.0f, pd.color.g / 255.0f, pd.color.b / 255.0f, pd.color.a / 255.0f);
    
    float tx1 = (texture % 64) * 64.0f / atlas.getWidth(), tx2 = (texture % 64 + 1) * 64.0f / atlas.getHeight();
    float ty1 = int(texture / 64) * 64.0f / atlas.getWidth(), ty2 = (int(texture / 64) + 1) * 64.0f / atlas.getHeight();

    if (is_spherical) {
      double lat = v.y / 180.0 * M_PI, lon = v.x / 180.0 * M_PI;
      v.x = -earth_radius * cos(lat) * cos(lon);
      v.y = earth_radius * sin(lat);
      v.z = earth_radius * cos(lat) * sin(lon);      
    }
        
    *(current_data++) = { glm::vec2(tx1, ty1), color, normal, glm::vec3(v.x - size / 2, v.y + size / 2, v.z) };
    *(current_data++) = { glm::vec2(tx1, ty2), color, normal, glm::vec3(v.x - size / 2, v.y - size / 2, v.z) };
    *(current_data++) = { glm::vec2(tx2, ty2), color, normal, glm::vec3(v.x + size / 2, v.y - size / 2, v.z) };
    *(current_data++) = { glm::vec2(tx2, ty1), color, normal, glm::vec3(v.x + size / 2, v.y + size / 2, v.z) };

    *current_index++ = idx + 0;
    *current_index++ = idx + 1;
    *current_index++ = idx + 3;
    *current_index++ = idx + 1;
    *current_index++ = idx + 2;
    *current_index++ = idx + 3;

    idx += 4;
  }

  vbo.setDrawType(VBO::TRIANGLES);
  vbo.uploadIndices(indices.get(), getNodeCount() * 6 * sizeof(unsigned int));
  vbo.upload(VBO::T2F_C4F_N3F_V3F, data.get(), getNodeCount() * 4 * sizeof(vbo_data2_s));
}

void
Graph::createLabelVBO(VBO & vbo, const TextureAtlas & atlas, float node_scale) const {
  unsigned int label_count = 0;
  for (unsigned int i = 0; i < getNodeCount(); i++) {
    auto & data = getNodeData(i);
    auto & sd = getNodeSecondaryData(i);
    if (data.flags & NODE_LABEL_VISIBLE && sd.label_texture) label_count++;
  }

  // cerr << "creating label vbo (n = " << label_count << ")\n";
  
  if (!label_count) return;
  
  std::unique_ptr<billboard_data_s[]> data(new billboard_data_s[4 * label_count]);
  std::unique_ptr<unsigned int[]> indices(new unsigned int[6 * label_count]);

  billboard_data_s * current_data = data.get();
  unsigned int * current_index = indices.get();

  int idx = 0;
  const table::Column & user_type = getNodeData()["type"];
  unsigned int actual_label_count = 0;
  
  for (unsigned int i = 0; i < getNodeCount(); i++) {
    auto & pd = getNodeData(i);
    auto & sd = getNodeSecondaryData(i);
    if (!(pd.flags & NODE_LABEL_VISIBLE && sd.label_texture)) continue;
    
    auto & pos = getPosition(i);
    // unsigned int flags = 0;
    int offset_x = 0, offset_y = 0;
    bool text_center = false;
    bool text_middle = true;

    if (label_style == LABEL_DARK_BOX) {
      float node_size = 0.0f;
      offset_y -= 0.8 * (3.0 + 2.0 * node_size);
      text_center = true;
    }
        
    auto & tp = atlas.getTexturePos(sd.label_texture);

    double x = 0, y = 0;
        
    if (text_center) x -= tp.width / 2.0f;
    if (text_middle) y -= tp.height / 2.0f;
    // else y -= tp.height;
    
    x += offset_x;
    y += offset_y;
        
    float tx1 = (float)tp.x / atlas.getWidth(), ty1 = (float)tp.y / atlas.getHeight();
    float tx2 = (float)(tp.x + tp.width) / atlas.getWidth(), ty2 = (float)(tp.y + tp.height) / atlas.getHeight();
    
    *(current_data++) = { pos, glm::packHalf2x16(glm::vec2(x, y + tp.height)), glm::packHalf2x16(glm::vec2(tx1, ty1)) };
    *(current_data++) = { pos, glm::packHalf2x16(glm::vec2(x, y)), glm::packHalf2x16(glm::vec2(tx1, ty2)) };
    *(current_data++) = { pos, glm::packHalf2x16(glm::vec2(x + tp.width, y)), glm::packHalf2x16(glm::vec2(tx2, ty2)) };
    *(current_data++) = { pos, glm::packHalf2x16(glm::vec2(x + tp.width, y + tp.height)), glm::packHalf2x16(glm::vec2(tx2, ty1)) };

    *current_index++ = idx + 0;
    *current_index++ = idx + 1;
    *current_index++ = idx + 3;
    *current_index++ = idx + 1;
    *current_index++ = idx + 2;
    *current_index++ = idx + 3;

    idx += 4;
    actual_label_count++;
  }

  if (label_count != actual_label_count) {
    cerr << "WARNING: label_count changed" << endl;
  }

  vbo.setDrawType(VBO::TRIANGLES);
  vbo.uploadIndices(indices.get(), label_count * 6 * sizeof(unsigned int));
  vbo.upload(VBO::BILLBOARDS, data.get(), label_count * 4 * sizeof(billboard_data_s));
}

// Gauss-Seidel relaxation for links
void
Graph::relaxLinks() {
  double avg_edge_weight = total_edge_weight / getEdgeCount();
  // cerr << "avg edge weight = " << avg_edge_weight << endl;
  auto end = end_edges();
  for (auto it = begin_edges(); it != end; ++it) {
    int s = it->tail, t = it->head;
    auto & pd1 = getNodeData(s), & pd2 = getNodeData(t);
    bool fixed1 = pd1.flags & NODE_FIXED_POSITION;
    bool fixed2 = pd2.flags & NODE_FIXED_POSITION;
    if (fixed1 && fixed2) continue;      
    glm::vec3 & pos1 = pd1.position, & pos2 = pd2.position;
    glm::vec3 d = pos2 - pos1;
    float l = glm::length(d);
    if (l < EPSILON) continue;
    auto & sd1 = getNodeSecondaryData(s), & sd2 = getNodeSecondaryData(t);
    if (sd1.cluster_id != sd2.cluster_id) continue;
    // d *= getAlpha() * it->weight * link_strength * (l - link_length) / l;
    d *= getAlpha2(); // * fabsf(it->weight) / avg_edge_weight;
    float w1 = sd1.type == NODE_HASHTAG ? 0 : pd1.size;
    float w2 = sd2.type == NODE_HASHTAG ? 0 : pd2.size;
    float k;
    if (fixed1) {
      k = 1.0f;
    } else if (fixed2) {
      k = 0.0f;
    } else {
#if 1
      k = w1 / (w1 + w2);
#else
      k = 0.5f;
#endif
    }
    pos2 -= d * k;
    pos1 += d * (1 - k);
  }
}

void
Graph::applyGravity(float gravity) {
  float k = getAlpha2() * gravity;
  if (k > EPSILON) {
    int n1 = getNodeCount();
    for (int i = 0; i < n1; i++) {
      auto & pd = getNodeData(i);
      if (pd.size > 0 && !(pd.flags & NODE_FIXED_POSITION)) {
	auto & sd = getNodeSecondaryData(i);
	
	glm::vec3 pos = pd.position;
	glm::vec3 origin;
	if (sd.cluster_id >= 0) origin = getClusterAttributes(sd.cluster_id).position;
	pos -= origin;
	float d = glm::length(pos);
	if (d > 0.001) {
	  pos -= pos * (k * sqrtf(d) / d);
	  pos += origin;
	  pd.position = pos;
	}
      }
    }

    int n2 = getClusterCount();
    for (int i = 0; i < n2; i++) {
      auto & pd = getClusterAttributes(i);
      glm::vec3 pos = pd.position;
      float d = glm::length(pos);
      if (d > 0.0001) {
	pos -= pos * (k * sqrtf(d) / d);
	pd.position = pos;
      }
    }
  }
}

void
Graph::applyDragAndAge(RenderMode mode, float friction) {
  int n = getNodeCount();
  for (int i = 0; i < n; i++) {
    auto & pd = getNodeData(i);
    glm::vec3 & pos = pd.position, & ppos = pd.prev_position;
    
    glm::vec3 new_pos = pos - (ppos - pos) * friction;
    if (mode == RENDERMODE_2D) {
      new_pos.z = 0;
    }
    pd.prev_position = pos;
    pd.position = new_pos;
    pd.age += 1.0f / 50.0f;
  }

  n = getClusterCount();
  for (int i = 0; i < n; i++) {
    auto & pd = getClusterAttributes(i);
    glm::vec3 & pos = pd.position, & ppos = pd.prev_position;
    
    glm::vec3 new_pos = pos - (ppos - pos) * friction;
    if (mode == RENDERMODE_2D) {
      new_pos.z = 0;
    }
    pd.prev_position = pos;
    pd.position = new_pos;    
  }
  version++;
}

int
Graph::getFaceId(short source_id, long long source_object_id) const {
  auto it = getFaceCache().find(skey(source_id, source_object_id));
  if (it != getFaceCache().end()) {
    return it->second;
  }
  return -1;
}

int
Graph::pickNode(const DisplayInfo & display, int x, int y, float node_scale) const {
  int best_i = -1;
  float best_d = 0;
  glm::vec2 ppos(x, y);
  for (int i = 0; i < getNodeCount(); i++) { 
    auto & pd = getNodeData(i);
    glm::vec3 tmp1 = display.project(pd.position);
    glm::vec3 tmp2 = display.project(pd.position + glm::vec3(pd.size / 2.0f / node_scale, 0.0f, 0.0f));
    glm::vec2 pos1(tmp1.x, tmp1.y);
    glm::vec2 pos2(tmp2.x, tmp2.y);
    glm::vec2 tmp3 = pos2 - pos1;
    float diam = glm::length(tmp3);
    pos1 -= ppos;
    float d = glm::length(pos1) - diam;
    if (d <= 0 && (best_i == -1 || d < best_d)) {
      best_i = i;
      best_d = d;
    }
  }
  return best_i;
}

void
Graph::extractLocationGraph(Graph & target_graph) {
  map<string, int> node_mapping;
  target_graph.getNodeData().addTextColumn("name");

  // Column & sentiment = target_graph.getNodeData().addDoubleColumn("sentiment");

  const Column & lat_column = getNodeData()["latitude"], & lon_column = getNodeData()["longitude"];  
 
  map<int, map<int, int> > seen_edges;
  // map<int, pair<float, unsigned int> > node_sentiments;
  
  auto end = end_edges();
  for (auto it = begin_edges(); it != end; ++it) {
    // float edge_sentiment = it->sentiment;
    pair<int, int> np(it->tail, it->head);

    double lon1 = lon_column.getDouble(np.first), lat1 = lat_column.getDouble(np.first);
    double lon2 = lon_column.getDouble(np.second), lat2 = lat_column.getDouble(np.second);
    
    if (lon1 == 0 && lat1 == 0) {
      np.first = -1;
    } else {
      string key1 = to_string(lon1) + "/" + to_string(lat1);
            
      auto it = node_mapping.find(key1);
      if (it != node_mapping.end()) {
	np.first = it->second;
      } else {
	int new_node_id = target_graph.addNode();
	node_mapping[key1] = new_node_id;
	glm::vec3 tmp((float)lon1, (float)lat1, 0);
	target_graph.setPositions(new_node_id, pair<glm::vec3, glm::vec3>(tmp, tmp));
	np.first = new_node_id;
      }

      // pair<float, unsigned int> & sn = node_sentiments[np.first];
      // sn.first += edge_sentiment;
      // sn.second++;
      
      NodeType type = getNodeSecondaryData(np.second).type;
      assert(type != NODE_URL && type != NODE_HASHTAG);
    }
    
    if (lon2 == 0 && lat2 == 0) {
      np.second = -1;
    } else {
      string key2 = to_string(lon2) + "/" + to_string(lat2);
      auto it = node_mapping.find(key2);
      if (it != node_mapping.end()) {
	np.second = it->second;
      } else {
	int new_node_id = target_graph.addNode();
	node_mapping[key2] = new_node_id;
	glm::vec3 tmp((float)lon2, (float)lat2, 0);
	target_graph.setPositions(new_node_id, pair<glm::vec3, glm::vec3>(tmp, tmp));
	np.second = new_node_id;
      }
    }

    if (np.first >= 0 && np.second >= 0 && np.first != np.second) {
      map<int, map<int, int> >::iterator it1;
      map<int, int>::iterator it2;
      if ((it1 = seen_edges.find(np.first)) != seen_edges.end() &&
	  (it2 = it1->second.find(np.second)) != it1->second.end()) {
	int edge = it2->second;
	target_graph.getEdgeAttributes(edge).weight += 1.0f;
	// weight.setValue(edge, weight.getInt(edge) + 1);
      } else {
	int edge = seen_edges[np.first][np.second] = target_graph.addEdge(np.first, np.second);	  
	// weight.setValue(edge, 1);
      }
    }
  }
  
  target_graph.setSRID(4326);
  target_graph.setNodeSizeMethod(getNodeSizeMethod());
  target_graph.updateAppearance();
  target_graph.setNodeVisibility(true);
  target_graph.setEdgeVisibility(true);
  target_graph.setRegionVisibility(false);
  target_graph.setLabelVisibility(true);  
}

static bool compareCameraDistance(const pair<int, float> & a, const pair<int, float> & b) {
  return a.second > b.second;
}

vector<int>
Graph::createSortedNodeIndices(const glm::vec3 & camera_pos) const {
  vector<pair<int, float> > v;
  for (unsigned int i = 0; i < getNodeCount(); i++) {
    glm::vec3 d = node_geometry[i].position - camera_pos;
    float camera_distance = glm::dot(d, d);
    v.push_back(pair<int, float>(i, camera_distance));
  }
  sort(v.begin(), v.end(), compareCameraDistance);
  vector<int> v2;
  for (auto p : v) {
    v2.push_back(p.first);
  }
  return v2;
}

void
Graph::simplifyWithClusters(const std::vector<int> & clusters, Graph & target_graph) {
  map<int, int> created_clusters;
  map<int, map<int, int> > seen_edges;
  
  for (int n1 = 0; n1 < getNodeCount(); n1++) {
    int partition1 = clusters[n1];
    
    auto it1 = created_clusters.find(partition1);
    int cluster1;
    if (it1 != created_clusters.end()) {
      cluster1 = it1->second;
    } else {
      created_clusters[partition1] = cluster1 = target_graph.addNode();
    }

    int edge = getNodeFirstEdge(n1);
    while (edge != -1) {
      auto & ed = getEdgeAttributes(edge);
      int n2 = getEdgeTargetNode(edge);
      int partition2 = clusters[n1];

      auto it2 = created_clusters.find(partition2);
      int cluster2;
      if (it2 != created_clusters.end()) {
	cluster2 = it2->second;
      } else {
	created_clusters[partition2] = cluster2 = target_graph.addNode();
      }

      assert(n1 != n2);
      
      map<int, map<int, int> >::iterator eit1;
      map<int, int>::iterator eit2;
      if ((eit1 = seen_edges.find(n1)) != seen_edges.end() &&
	  (eit2 = eit1->second.find(n2)) != eit1->second.end()) {
	auto & attr = target_graph.getEdgeAttributes(eit2->second);
	attr.weight += ed.weight;
      } else {
	int new_edge = target_graph.addEdge(n1, n2);
	seen_edges[n1][n2] = new_edge;
	auto & attr = target_graph.getEdgeAttributes(new_edge);
      }
      
      edge = getNextNodeEdge(edge);
    }    
  }
}

void
Graph::selectNodes(int input_node, int depth) {
  if (input_node == -1) {
    has_node_selection = false;
    for (int n = 0; n < getNodeCount(); n++) {
      node_geometry[n].flags |= NODE_SELECTED;
    }
  } else {
    has_node_selection = true;
    for (int n = 0; n < getNodeCount(); n++) {
      node_geometry[n].flags &= ~NODE_SELECTED;
    }
    
    if (input_node >= 0) { // can be -2 to select none
      map<int, set<int> > all_edges;
      
      auto end = end_edges();
      for (auto it = begin_edges(); it != end; ++it) {
	all_edges[it->tail].insert(it->head);
	all_edges[it->head].insert(it->tail);
      }
      
      set<int> seen_nodes;
      list<pair<int, int> > node_queue;
      node_queue.push_back(make_pair(input_node, 0));
      
      while (!node_queue.empty()) {
	int node_id = node_queue.front().first;
	int node_depth = node_queue.front().second;
	node_queue.pop_front();
	
	if (!seen_nodes.count(node_id)) {
	  seen_nodes.insert(node_id);
	  
	  node_geometry[node_id].flags |= NODE_SELECTED;
	  
	  if (node_depth < depth) {
	    map<int, set<int> >::iterator node_edges = all_edges.find(node_id);
	    if (node_edges != all_edges.end()) {
	      for (auto succ : node_edges->second) {
		node_queue.push_back(make_pair(succ, node_depth + 1));
	      }
	    }
	  }
	}
      }
    }
  }

  version++;
}

std::vector<int>
Graph::getNestedGraphIds() const {
  std::vector<int> v;
  for (int i = 0; i < getNodeCount(); i++) {
    auto & graph = node_geometry2[i].nested_graph;
    if (graph.get()) v.push_back(graph->getId());
  }
  return v;
}

std::vector<int>
Graph::getLocationGraphs() const {
  std::vector<int> v;
  for (int i = 0; i < getNodeCount(); i++) {
    auto & graph = node_geometry2[i].nested_graph;
    if (graph.get() && graph->getLocation().get()) v.push_back(graph->getId());
  }
  return v;
}

void
Graph::refreshLayouts() {
  cerr << "resume after refreshLayouts\n";
  resume2();
  for (auto & g : final_graphs) {
    g->resume2();
  }
  for (int i = 0; i < getNodeCount(); i++) {
    auto & graph = node_geometry2[i].nested_graph;
    if (graph.get()) graph->refreshLayouts();
  }
}

std::vector<std::shared_ptr<Graph> >
Graph::getNestedGraphs() {
  std::vector<std::shared_ptr<Graph> > v;
  for (int i = 0; i < getNodeCount(); i++) {
    auto & graph = node_geometry2[i].nested_graph;
    if (graph.get()) {
      v.push_back(graph);
    }
  }
  return v;
}

int
Graph::getGraphNodeId(int graph_id) const {
  for (int i = 0; i < getNodeCount(); i++) {
    auto & graph = node_geometry2[i].nested_graph;
    if (graph.get() && graph->getId() == graph_id) return i;
  }
  return -1;
}

void
Graph::createClusters() {
#ifndef _WIN32
  double precision = 0.000001;
  
  cerr << "creating arrays, precision = " << precision << "\n";
  
  double total_weight = 0;
  vector<unsigned long long> degrees;
  vector<unsigned int> links;
  vector<float> weights;

  cerr << "populating arrays\n";

  for (int v = 0; v < getNodeCount(); v++) {
    unsigned int degree = 0;
    int edge = getNodeFirstEdge(v);
    while (edge != -1) {
      edge_data_s & ed = getEdgeAttributes(edge);
      
      int succ = getEdgeTargetNode(edge);
      assert(succ >= 0 && succ < getNodeCount());

      links.push_back(succ);
      weights.push_back(ed.weight);
      total_weight += ed.weight;
      degree++;
      
      edge = getNextNodeEdge(edge);
    }
    degrees.push_back(degree);
  }

  cerr << "accumulating\n";

  for (int i = 1; i < degrees.size(); i++) {
    degrees[i] += degrees[i - 1];
  }

  cerr << "accumulation done, degrees.size() = " << degrees.size() << ", .back() = " << degrees.back() << ", nodes = " << getNodeCount() << ", edges = " << getEdgeCount() << endl;

  assert(degrees.size() == getNodeCount());
  assert(degrees.back() == getEdgeCount() ||
	 degrees.back() == 2 * getEdgeCount());
  
  cerr << "creating communities\n";

  ColorProvider colors(ColorProvider::CHART2);
  
  BinaryGraph binary_graph(getNodeCount(), weights.size(), total_weight, degrees, links, weights);
  
  Community c(binary_graph, -1, precision);
  double mod = c.modularity(), new_mod;
  int level = 0;
  int display_level = -1;
  bool improvement = true;
  bool is_first = true;
  BinaryGraph g;
  do {
    cerr << "level " << level << ":\n";
    cerr << "  network size: " 
	 << c.getGraph().getNodeCount() << " nodes, " 
	 << c.getGraph().getLinkCount() << " links, " << endl;
    improvement = c.one_level();
    new_mod = c.modularity();
    if (++level == display_level) g.display();
    if (display_level == -1) c.display_partition();

    if (is_first) {
      auto partition = c.getRenumberedPartition();
      unsigned int n = 0;
      for (auto a : partition) if (a + 1 >= n) n = a + 1;
      assert(partition.size() == getNodeCount());

      glm::vec3 c1(1.0, 0.0, 0.0), c2(1.0, 1.0, 0.0);
      for (int i = 0; i < getNodeCount(); i++) {
	int p = partition[i];
	// float f = float(p) / (n - 1);
	// glm::vec3 c = glm::normalize(glm::mix(c1, c2, f));
	// cerr << "assigning colors (" << i << "/" << n << ", p = " << p << ", f = " << f << ")\n";
	auto color = colors.getColorByIndex(p);
	setNodeColor2(i, color);
	while (p >= getClusterCount()) {
	  cerr << "adding cluster\n";
	  int cluster_id = addCluster();
	  setClusterColor(cluster_id, color);
	}
	setNodeCluster(i, p);
      }
      is_first = false;
    }
    
    g = c.partition2graph_binary();
    c = Community(g, -1, precision);
    
    cerr << "  modularity increased from " << mod << " to " << new_mod << endl;
    mod = new_mod;
  } while (improvement);  

  incVersion();
#endif
}

void
Graph::storeChangesFromFinal() {
  for (auto & g : final_graphs) {
    for (int j = 0; j < g->getNodeCount(); j++) {
      auto & sd = g->getNodeSecondaryData(j);
      if (sd.orig_node_id >= 0) {
	auto & pd = g->getNodeData(j);
	assert(sd.orig_node_id < getNodeCount());
	auto & pd2 = getNodeData(sd.orig_node_id);
	auto & sd2 = getNodeSecondaryData(sd.orig_node_id);
	pd2.position = pd.position;
	pd2.prev_position = pd.prev_position;
	pd2.color = pd.color;
	pd2.age = pd.age;
	pd2.flags = pd.flags;
	sd2.label = sd.label;
	sd2.cluster_id = sd.cluster_id;
	if (sd.label_texture) sd2.label_texture = sd.label_texture;
	sd2.label_visibility_val = sd.label_visibility_val;
      }
    }
  
    setClusterVisibility(g->getClusterVisibility());
    setNodeVisibility(g->getNodeVisibility());
    setEdgeVisibility(g->getEdgeVisibility());
    setRegionVisibility(g->getRegionVisibility());
    setLabelVisibility(g->getLabelVisibility());
    setDefaultNodeColor(g->getDefaultNodeColor());
    setDefaultEdgeColor(g->getDefaultEdgeColor());
    setDefaultRegionColor(g->getDefaultRegionColor());
    setAlpha3(g->getAlpha2());
    // setAlphaVelocity2(g->getAlphaVelocity2());
  }
}

unsigned int
Graph::getSuitableFinalGraphCount() const {
  if (getNodeCount() >= 100 || 1) {
    return 2;
  } else {
    return 1;
  }
}

bool
Graph::updateSelection2(time_t start_time, time_t end_time, float start_sentiment, float end_sentiment) {
  if (getPersonality() != Graph::SOCIAL_MEDIA || !isTemporal()) {
    return false;
  }

  unsigned int count = getSuitableFinalGraphCount();
  if (final_graphs.size() != count) {
    storeChangesFromFinal();
    final_graphs.clear();
    cerr << "CREATING FINALs!\n";
    if (count == 2) {
      auto g1 = createSimilar();
      auto g2 = createSimilar();
      g1->setMinSignificance(10.0f);
      g1->setMinScale(1.0f);
      addFinalGraph(g1);
      addFinalGraph(g2);
    } else {
      addFinalGraph(createSimilar());
    }
    statistics.clear();
  } else if (!final_graphs.front()->hasPosition()) {
    cerr << "RESETTING EVERYTHING!\n";
    if (final_graphs.front()->hasNodeSelection()) {
      selectNodes(-2);
    } else {
      selectNodes();
    }
    
    statistics.clear();
    
    for (int j = 0; j < getNodeCount(); j++) {
      auto & pd = getNodeData(j);
      auto & sd = getNodeSecondaryData(j);
      pd.age = -2.0f;
      pd.flags = NODE_SELECTED;
      sd.label_visibility_val = 0;
    }

    storeChangesFromFinal();
  }
    
  statistics.setSentimentRange(start_sentiment, end_sentiment);

  bool changed = false;
  for (unsigned int i = 0; i < final_graphs.size(); i++) {
    auto & g = final_graphs[i];
    Graph * base_graph = i > 0 ? final_graphs[i].get() : 0;
    if (g->updateData(start_time, end_time, start_sentiment, end_sentiment, *this, statistics, base_graph)) {
      g->updateAppearance();
      g->incVersion();
      setLocationGraphValid(false);
      incVersion();
      resume2();
      g->resume2();
      changed = true;
    }
  }
  return changed;         
}

void
Graph::setNodeColor2(int i, const canvas::Color & c) {
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

void
Graph::setClusterColor(int i, const canvas::Color & c) {
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
  setClusterColor(i, tmp);
}

void
Graph::calculateEdgeCentrality() {
  int num_edges = edges.size();
  vector<double> betweenness_data(num_edges, 0);

  cerr << "edge centrality: n = " << num_edges << endl;
  
  for (unsigned int source = 0; source < num_edges; source++) {
    vector<double> sigma_data(num_edges, 0);
    vector<double> delta_data(num_edges, 0);
    vector<int> distance_data(num_edges, -1);
    vector<list<int> > predecessors(num_edges, list<int>());
    
    sigma_data[source] = 1;
    distance_data[source] = 0;
    
    list<int> stack, queue;
    queue.push_back(source); 
   
    while (!queue.empty()) {
      int e = queue.front();
      queue.pop_front();
      stack.push_back(e);

      auto & ed = getEdgeAttributes(e);
      int target_node = ed.head;

      int succ = getNodeFirstEdge(target_node);
      while (succ != -1) {
	if (distance_data[succ] < 0) { // succ found for first time
	  queue.push_back(succ);
	  distance_data[succ] = distance_data[e] + 1;
	}
	if (distance_data[succ] == distance_data[e] + 1) { // shortest path to succ via v
	  sigma_data[succ] += sigma_data[e];
	  predecessors[succ].push_back(e);
	}
	succ = getNextNodeEdge(succ);
      }
    }
    
    for (list<int>::reverse_iterator it2 = stack.rbegin(); it2 != stack.rend(); it2++) {
      int w = *it2;
      for (list<int>::iterator it3 = predecessors[w].begin(); it3 != predecessors[w].end(); it3++) {
	int pre = *it3;
	delta_data[pre] += (sigma_data[pre] / sigma_data[w]) * (1 + delta_data[w]);
      }
      if (w != source) {
	betweenness_data[w] += delta_data[w];
      }
    }
  }
  
  // normalise & clean up
    
  double factor = (num_edges - 1) * (num_edges - 2);
  
  double sum = 0;
  total_edge_weight = 0;
  for (unsigned int i = 0; i < num_edges; i++) {
    sum += betweenness_data[i];
    double w = betweenness_data[i] == 0 ? 0.1 : 1 + log(1 + betweenness_data[i]);
    if (betweenness_data[i] != 0) total_edge_weight += w;
    getEdgeAttributes(i).weight = w;
  }

  total_edge_weight *= 4;

  cerr << "sum = " << sum << ", avg =  " << sum / num_edges << endl;

  incVersion();
  randomizeGeometry();
  updateAppearance();
  resume2();
}

struct label_data_s {
  glm::vec2 pos;
  float size;
  int index;
  bool is_selected;
};

static bool compareSize(const label_data_s & a, const label_data_s & b) {
  return a.size > b.size;
}

bool
Graph::updateLabelVisibility(const DisplayInfo & display, bool reset) {
  vector<label_data_s> all_labels;
  
  for (int i = 0; i < (int)getNodeCount(); i++) {
    auto & sd = getNodeSecondaryData(i);
    if (!sd.label.empty() || getDefaultSymbolId()) {
      auto & pd = getNodeData(i);
      if (pd.age >= 0 && (display.isPointVisible(pd.position) ||
			  getNodeLabelVisibility(i))) {
	glm::vec3 win = display.project(pd.position);
	bool is_selected = false; // graph.getId() == selected_node.first && i == selected_node.second;
	all_labels.push_back({ glm::vec2(win.x, win.y), pd.size, i, is_selected });
      }
    }
  }

  bool changed = false;
  
  if (all_labels.size() <= 10) {
    for (vector<label_data_s>::iterator it = all_labels.begin(); it != all_labels.end(); it++) {
      setNodeLabelVisibilityValue(it->index, 1);
      changed |= setNodeLabelVisibility(it->index, true);
    }
  } else {
    sort(all_labels.begin(), all_labels.end(), compareSize);
  
    vector<label_data_s> drawn_labels;
    // const Rect2d & region = getContentRegion();

    for (vector<label_data_s>::iterator it = all_labels.begin(); it != all_labels.end(); it++) {
      bool fits = true;
      auto & my_pos = it->pos;
      bool reset_this = reset;
      if (it->is_selected) {
	reset_this = true;
#if 0
      } else if (my_pos.x < region.left || my_pos.y < region.bottom || my_pos.x > region.right || my_pos.y > region.top) {
	fits = false;
#endif	
      } else {
	for (vector<label_data_s>::const_iterator it2 = drawn_labels.begin(); it2 != drawn_labels.end(); it2++) {
	  auto & other_pos = it2->pos;
	  
	  float dx = fabsf(my_pos.x - other_pos.x);
	  float dy = fabsf(my_pos.y - other_pos.y);
	  
	  if (dx < 200 && dy < 100) {
	    fits = false;
	  }
	}
      }
      if (fits) {
	drawn_labels.push_back(*it);
      }
      if (reset_this) {
	setNodeLabelVisibilityValue(it->index, fits ? 1 : 0);
	changed |= setNodeLabelVisibility(it->index, fits);
      } else {
	changed |= updateNodeLabelValues(it->index, fits ? 1.00f : -1.00f);
      }
    }
  }

  if (changed) {
    // cerr << "labels changed!\n";
    incVersion();
    return true;
  } else {
    return false;
  }
}

Graph *
Graph::getGraphById2(int graph_id) {
  if (graph_id == getId()) {
    return this;
  } else if (hasSubGraphs()) {
    for (int i = 0; i < getNodeCount(); i++) {
      auto & graph = node_geometry2[i].nested_graph;
      if (graph.get()) {
	Graph * r = graph->getGraphById2(graph_id);
	if (r) return r;
      }
    }
  }
  return 0;
}

const Graph *
Graph::getGraphById2(int graph_id) const {
  if (graph_id == getId()) {
    return this;
  } else if (hasSubGraphs()) {
    for (int i = 0; i < getNodeCount(); i++) {
      auto & graph = node_geometry2[i].nested_graph;
      if (graph.get()) {
	const Graph * r = graph->getGraphById2(graph_id);
	if (r) return r;
      }
    }
  }
  return 0;
}

GraphRefR
Graph::getGraphForReading(int graph_id) const {
  const Graph * graph = getGraphById2(graph_id);
  return GraphRefR(graph); 
}

GraphRefW
Graph::getGraphForWriting(int graph_id) {
  Graph * graph = getGraphById2(graph_id);
  return GraphRefW(graph); 
}

skey
Graph::getNodeKey(int node_id) const {
  const table::Column & source_id_column = getNodeData()["source"];
  const table::Column & id_column = getNodeData()["id"];
  
  short source_id = source_id_column.getInt(node_id);
  long long source_object_id = 0;

  if (source_id) {
    source_object_id = id_column.getInt64(node_id);
  } else {
    source_object_id = node_id;
  }
  
  skey key(source_id, source_object_id);
  return key;
}

void
Graph::setNormal(int i, const glm::vec4 & v) {
  node_geometry[i].normal = glm::packSnorm3x10_1x2(v);
  version++;
}

void
Graph::invalidateVisibleNodes() {
  storeChangesFromFinal();
  final_graphs.clear();
  for (int i = 0; i < getNodeCount(); i++) {
    auto & graph = node_geometry2[i].nested_graph;
    if (graph.get()) {
      graph->invalidateVisibleNodes();      
    }
  }
}

void
Graph::setNodeTexture(const skey & key, int texture) { 
  auto it2 = getNodeCache().find(key);
  if (it2 != getNodeCache().end()) {
    setNodeTexture(it2->second, texture);
  }
  for (auto & g : final_graphs) {
    g->setNodeTexture(key, texture);
  }
}

void
Graph::setLabelTexture(const skey & key, int texture) {
  auto it2 = getNodeCache().find(key);
  if (it2 != getNodeCache().end()) {
    setLabelTexture(it2->second, texture);
  }
  for (auto & g : final_graphs) {
    g->setLabelTexture(key, texture);
  }
}

std::shared_ptr<Graph>
Graph::getFinal(float scale) {
  cerr << "getting final for scale " << scale << endl;
  for (auto & g : final_graphs) {
    if (scale >= g->getMinScale()) {
      cerr << "  min_scale = " << g->getMinScale() << ", min_sig = " << g->getMinSignificance() << endl;
      return g;
    }
  }
  return std::shared_ptr<Graph>(0);
}

const std::shared_ptr<const Graph>
Graph::getFinal(float scale) const {
  cerr << "getting final for scale " << scale << ", levels = " << final_graphs.size() << ", nodes = " << getNodeCount() << endl;
  for (auto & g : final_graphs) {
    if (!g->getMinScale() || scale < g->getMinScale()) {
      cerr << "  min_scale = " << g->getMinScale() << ", min_sig = " << g->getMinSignificance() << endl;
      return g;
    }
  }
  return std::shared_ptr<Graph>(0);
}

