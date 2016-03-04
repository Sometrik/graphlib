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
#include <unordered_set>

#ifndef _WIN32
#include "community/BinaryGraph.h"
#include "community/Community.h"
#endif

#define EPSILON 0.0000000001

using namespace std;

int Graph::next_id = 1;

Graph::Graph(int _id)
  : id(_id),
    node_color(0.0f, 0.0f, 0.0f, 0.0f),
    edge_color(0.0f, 0.0f, 0.0f, 0.0f),
    face_color(0.0f, 0.0f, 0.0f, 0.0f)
{
  if (!id) id = next_id++;
}


Graph::~Graph() {
  
}

Graph::Graph(const Graph & other)
  : nodes(other.nodes), // copy node array by reference
    faces(other.faces),
    face_attributes(other.face_attributes),
    edge_attributes(other.edge_attributes),
    version(other.version),
    source_id(other.source_id)
{
  id = next_id++;
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

static table::Column * sort_col = 0;

static bool compareRows(const int & a, const int & b) {
  return sort_col->compare(a, b);
}

#if 0
void
Graph::setFaceColorByColumn(int column) {
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
#endif

void
Graph::randomizeGeometry(bool use_2d) {
  assert(!nodes->hasSpatialData());
  unsigned int num_nodes = getNodeArray().size();
  for (unsigned int i = 0; i < num_nodes; i++) {
    getNodeArray().setRandomPosition(i);
        
    auto & graph = getNodeArray().node_geometry[i].nested_graph;
    if (graph.get() && !graph->nodes->hasSpatialData()) {
      graph->randomizeGeometry(use_2d);       
    }
  }
  mbr = Rect2d(-50, -50, +50, +50);
}

void
Graph::createRegionVBO(VBO & vbo) const {
  if (!getFaceCount()) {
    return;
  }

  assert(getFaceVisibility());
    
  if (nodes->hasArcData()) {
    vector<arc_data_2d_s> new_geometry;
    vector<unsigned int> front_indices, back_indices;

    auto & arc_geometry = nodes->getArcGeometry();
    vector<int> stored_arcs;
    stored_arcs.resize(arc_geometry.size());
		       
    cerr << "creating complex region VBO, faces = " << getFaceCount() << "\n";
    for (unsigned int face = 0; face < getFaceCount(); face++) {   
      auto & this_color = getFaceAttributes(face).color;
      glm::vec4 color( this_color.r * 255.0f, this_color.g * 255.0f, this_color.b * 255.0f, this_color.a * 255.0f );
      if (!color.w || 1) color = getDefaultFaceColor();
      if (!color.w) color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
      
      list<list<int> > part_arcs;
      int edge = getFaceFirstEdge(face);
      int prev_node = -1;
      while (edge != -1) {
	auto & ed = getEdgeAttributes(edge);
	if (ed.head != prev_node) {
	  part_arcs.push_back(list<int>());
	}
	prev_node = ed.tail;
	assert(ed.arc);
	if (ed.arc) part_arcs.back().push_front(ed.arc);
	edge = getNextFaceEdge(edge);
      }
      assert(!part_arcs.empty());
      for (auto & arcs : part_arcs) {
	assert(!arcs.empty());
	if (!arcs.empty()) {
	  vector<int> part_indices;
	  for (auto & arc_id : arcs) {
	    int ai = abs(arc_id) - 1;
	    assert(ai >= 0 && ai < stored_arcs.size());
	    int ao = stored_arcs[ai];
	    auto & geometry = arc_geometry[ai].data;
	    if (!ao) {
	      ao = stored_arcs[ai] = new_geometry.size() + 1;
	      for (auto & v : geometry) {
		new_geometry.push_back({ (unsigned char)(color.x * 255), (unsigned char)(color.y * 255), (unsigned char)(color.z * 255), (unsigned char)(color.w * 255), glm::vec2((float)v.x, (float)v.y) });
	      }
	    }
	    bool rev = arc_id < 0;
	    for (int i = 0; i < geometry.size(); i++) {
	      part_indices.push_back(ao - 1 + (rev ? geometry.size() - 1 - i : i));
	    }
	  }
	  assert(!part_indices.empty());
	  for (int vi = 2; vi < part_indices.size(); vi++) {
	    int i1 = part_indices[0], i2 = part_indices[vi], i3 = part_indices[vi - 1];
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
	}	
      }
    }
    
    back_indices.insert(back_indices.begin(), front_indices.begin(), front_indices.end());
       
    vbo.upload(VBO::ARCS_2D, &(new_geometry.front()), new_geometry.size() * sizeof(arc_data_2d_s));
    vbo.uploadIndices(&(back_indices.front()), back_indices.size() * sizeof(unsigned int));    
  } else {
    cerr << "creating simple region VBO, faces = " << getFaceCount() << "\n";

    vector<arc_data_3d_s> new_geometry;
    vector<unsigned int> indices;
    glm::vec3 normal;

    for (unsigned int face = 0; face < getFaceCount(); face++) {   
      auto & this_color = getFaceAttributes(face).color;
      glm::vec4 color( this_color.r * 255.0f, this_color.g * 255.0f, this_color.b * 255.0f, this_color.a * 255.0f );
      if (!color.w) color = getDefaultFaceColor();
      if (!color.w) color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

      int first_index = new_geometry.size();
      int edge = getFaceFirstEdge(face);
      int vi = 0;
      while (edge != -1) {
	auto & ed = getEdgeAttributes(edge);
	auto & nd = nodes->getNodeData(ed.tail);
	if (vi >= 2) {
	  indices.push_back(first_index);
	  indices.push_back(first_index + vi - 1);
	  indices.push_back(first_index + vi);
	}
	new_geometry.push_back({ (unsigned char)(color.x * 255), (unsigned char)(color.y * 255), (unsigned char)(color.z * 255), (unsigned char)(color.w * 255), nd.position, normal });
	vi++;
	edge = getNextFaceEdge(edge);
      }
    }

    if (!new_geometry.empty() && !indices.empty()) {
      vbo.upload(VBO::ARCS_3D, &(new_geometry.front()), new_geometry.size() * sizeof(arc_data_3d_s));
      vbo.uploadIndices(&(indices.front()), indices.size() * sizeof(unsigned int));
    } else {
      cerr << "skipping VBO for Graph " << getId() << "\n";
    }
  }
}

struct pair_hash {
  std::size_t operator()(const std::pair<int,int>& p) const {
    std::hash<int> int_hasher;
    return int_hasher(p.first) ^ int_hasher(p.second);
  }
};

void
Graph::createEdgeVBO(VBO & vbo) const {
  if (!getEdgeCount()) {
    return;
  }

  if (nodes->hasArcData()) {
    cerr << "loading arcs" << endl;
    unsigned int num_vertices = 0, num_indices = 0;
    auto & arc_geometry = nodes->getArcGeometry();
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
    
    vbo.upload(VBO::ARCS_2D, new_geometry.get(), vn * sizeof(arc_data_2d_s));
    vbo.uploadIndices(indices.get(), in * sizeof(unsigned int));
  } else {
    unsigned int ec = getEdgeCount();
    unsigned int asize = 2 * ec * sizeof(line_data_s);
    std::unique_ptr<line_data_s[]> new_geometry(new line_data_s[2 * ec]);
    // vector<unsigned int> node_mapping, indices;
    // node_mapping.resize(nodes->size());
    graph_color_s sel_color = { 100, 200, 255, 255 };
    graph_color_s def_color = { 200, 200, 200, 255 };
    auto & size_method = nodes->getNodeSizeMethod();
    unsigned int vn = 0;
    bool flatten = nodes->doFlattenHierarchy();
    unsigned int num_nodes = nodes->size();
    vector<bool> processed_edges;
    if (flatten) processed_edges.resize(num_nodes * num_nodes);
    auto end = end_edges();
    for (auto it = begin_edges(); it != end; ++it) {
      int tail = it->tail, head = it->head;
      if (flatten) {
	int l1 = 0, l2 = 0;
	for (int p = node_geometry3[tail].parent_node; p != -1; p = node_geometry3[p].parent_node) l1++;
	for (int p = node_geometry3[head].parent_node; p != -1; p = node_geometry3[p].parent_node) l2++;
	while ( 1 ) {
	  if (l1 > l2) {
	    tail = node_geometry3[tail].parent_node;
	    l1--;
	  } else if (l2 > l1) {
 	    head = node_geometry3[head].parent_node;
	    l2--;
	  } else if (node_geometry3[tail].parent_node == node_geometry3[head].parent_node) {
	    break;
	  } else {
	    l1--; l2--;
	    tail = node_geometry3[tail].parent_node;
	    head = node_geometry3[head].parent_node;
	  }
	}
	if (processed_edges[tail * num_nodes + head]) {
	  continue;
	}
	processed_edges[tail * num_nodes + head] = true;
	processed_edges[head * num_nodes + tail] = true;
      }
      if (tail == head) continue;
      auto & g1 = nodes->node_geometry[tail], & g2 = nodes->node_geometry[head];
	
      bool edge_selected = g1.isSelected() || g2.isSelected();
      glm::vec3 pos1 = g1.position, pos2 = g2.position;
      for (int p = getNodeTertiaryData(tail).parent_node; p != -1; p = getNodeTertiaryData(p).parent_node) {
	pos1 += nodes->getNodeData(p).position;
      }
      for (int p = getNodeTertiaryData(head).parent_node; p != -1; p = getNodeTertiaryData(p).parent_node) {
	pos2 += nodes->getNodeData(p).position;
      }
      glm::vec3 v = pos2 - pos1;
      float l = glm::length(v);
      if (l > 0.0001) v *= 1.0f / l;
	
      if (1) { // !i1) {
	auto & td = getNodeTertiaryData(tail);
	float size = size_method.calculateSize(td, total_indegree, total_outdegree, nodes->size());

	pos1 += v * size;

	auto color = edge_selected ? sel_color : def_color;
	
	float a = powf(it->weight / max_edge_weight, 0.9);
	line_data_s s = { (unsigned char)((255 * (1-a)) + color.r * a), (unsigned char)((255 * (1-a)) + color.g * a), (unsigned char)((255 * (1-a)) + color.b * a), (unsigned char)((255 * (1-a)) + color.a * a), pos1, td.age, 1.0f }; // g1.size
	// i1 = node_mapping[it->tail] = vn + 1;
	*((line_data_s*)(new_geometry.get()) + vn) = s;      
	vn++;
      }
      if (1) { // !i2) {
	auto & td = getNodeTertiaryData(tail);
	float size = size_method.calculateSize(td, total_indegree, total_outdegree, nodes->size());

	pos2 -= v * (size / l);

	auto color = edge_selected ? sel_color : def_color;
	float a = powf(it->weight / max_edge_weight, 0.9);
	// i2 = node_mapping[it->head] = vn + 1;
	line_data_s s = { (unsigned char)((255 * (1-a)) + color.r * a), (unsigned char)((255 * (1-a)) + color.g * a), (unsigned char)((255 * (1-a)) + color.b * a), (unsigned char)((255 * (1-a)) + color.a * a), pos2, td.age, 1.0f }; // g.size
	*((line_data_s*)(new_geometry.get()) + vn) = s;
	vn++;
      }
    }
    assert(vn <= 2 * ec);
    // cerr << "uploading edges: vn = " << vn << ", indices = " << indices.size() << endl;
    vbo.upload(VBO::EDGES, new_geometry.get(), vn * sizeof(line_data_s));
    // vbo.uploadIndices(&(indices.front()), indices.size() * sizeof(unsigned int));
  }
}

void
Graph::createNodeVBOForSprites(VBO & vbo) const {
  if (!getEdgeCount()) {
    return;
  }

  vector<node_vbo_s> new_geometry;
  graph_color_s parent_color = { 50, 50, 255, 0 };
  graph_color_s def_color = { 200, 200, 200, 255 };
  auto & size_method = nodes->getNodeSizeMethod();

  auto end = end_visible_nodes();
  for (auto it = begin_visible_nodes(); it != end; ++it) {
    auto & nd = nodes->getNodeData(*it);
    auto & td = getNodeTertiaryData(*it);
    const graph_color_s & col = td.child_count ? parent_color : def_color;
    float size = size_method.calculateSize(td, total_indegree, total_outdegree, nodes->size());
    auto pos = nd.position;
    for (int p = td.parent_node; p != -1; p = getNodeTertiaryData(p).parent_node) {
      pos += nodes->getNodeData(p).position;
    }
    new_geometry.push_back({ col.r, col.g, col.b, col.a, pos, td.age, size, nd.texture, nd.flags });
  }
  
  // cerr << "uploaded node vbo: edges = " << edge_count << ", nodes = " << new_geometry.size() << endl;
  
  vbo.upload(VBO::NODES, &(new_geometry.front()), new_geometry.size() * sizeof(node_vbo_s));
}

#if 0
vector<int>
Graph::createSortedNodeIndices(const glm::vec3 & camera_pos) const {
  vector<pair<int, float> > v;
  for (unsigned int i = 0; i < nodes->size(); i++) {
    glm::vec3 d = getNodeArray().node_geometry[i].position - camera_pos;
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
#endif
  
void
Graph::createNodeVBOForQuads(VBO & vbo) const {
  if (!nodes->size()) {
    return;
  }

  std::vector<node_billboard_vbo_s> new_geometry;
  std::vector<unsigned int> indices;
  auto & size_method = nodes->getNodeSizeMethod();

  static graph_color_s parent_color = { 50, 50, 255, 0 };
  static graph_color_s def_color = { 200, 200, 200, 255 };

  auto end = end_visible_nodes();
  for (auto it = begin_visible_nodes(); it != end; ++it) {
    auto & nd = nodes->getNodeData(*it);
    auto & td = getNodeTertiaryData(*it);
    float size = size_method.calculateSize(td, total_indegree, total_outdegree, nodes->size());
    bool visible = true;
    auto pos = nd.position;
    for (int p = td.parent_node; p != -1; p = getNodeTertiaryData(p).parent_node) {
      auto & ptd = nodes->getNodeData(p);
      if (!ptd.isOpen()) {
	visible = false;
	break;
      }
      pos += ptd.position;
    }

    const graph_color_s & col = td.child_count ? parent_color : def_color;
    float scaling = td.child_count ? 0.0 : 1.0;
  
    unsigned int base = new_geometry.size();
    new_geometry.push_back({ col.r, col.g, col.b, col.a, pos, td.age, size, scaling, nd.texture, nd.flags });
    new_geometry.push_back({ col.r, col.g, col.b, col.a, pos, td.age, size, scaling, nd.texture, nd.flags });
    new_geometry.push_back({ col.r, col.g, col.b, col.a, pos, td.age, size, scaling, nd.texture, nd.flags });
    new_geometry.push_back({ col.r, col.g, col.b, col.a, pos, td.age, size, scaling, nd.texture, nd.flags });

    indices.push_back(base + 0);
    indices.push_back(base + 1);
    indices.push_back(base + 3);
    indices.push_back(base + 1);
    indices.push_back(base + 2);
    indices.push_back(base + 3);
  }
  
  vbo.setDrawType(VBO::TRIANGLES);
  vbo.uploadIndices(&(indices.front()), indices.size() * sizeof(unsigned int));
  vbo.upload(VBO::NODE_BILLBOARDS, &(new_geometry.front()), new_geometry.size() * sizeof(node_billboard_vbo_s));
}

#define LABEL_FLAG_CENTER	1
#define LABEL_FLAG_MIDDLE	2
struct label_pos_s {
  glm::vec3 pos;
  float x, y;
  int texture;
  unsigned short flags;
  glm::vec4 color1, color2;
};

void
Graph::createLabelVBO(VBO & vbo, const TextureAtlas & atlas, float node_scale) const {  
  const table::Column & user_type = getNodeArray().getTable()["type"];
  vector<label_pos_s> labels;

  glm::vec4 black(0.0, 0.0, 0.0, 1.0), white(1.0, 1.0, 1.0, 1.0);
    
  for (unsigned int i = 0; i < getFaceCount(); i++) {
    auto & fd = getFaceAttributes(i);
    if (!(fd.flags & FACE_LABEL_VISIBLE && fd.label_texture)) continue;

    glm::vec3 pos(fd.centroid.x, fd.centroid.y, 0.0f);
    float x = 0.0f, y = 0.0f;

    unsigned short flags = 0;
    flags |= LABEL_FLAG_MIDDLE;

    labels.push_back({ pos, x, y, fd.label_texture, flags, black, white });
  }

  auto nodes_end = end_visible_nodes();
  for (auto it = begin_visible_nodes(); it != nodes_end; ++it) {
    auto & pd = getNodeArray().getNodeData(*it);
    if (!(pd.isLabelVisible() && pd.label_texture)) continue;

    auto & td = getNodeTertiaryData(*it);
    
    auto pos = pd.position;
    for (int p = td.parent_node; p != -1; p = getNodeTertiaryData(p).parent_node) {
      pos += nodes->getNodeData(p).position;
    }

    float offset_x = 0, offset_y = 0;
    unsigned short flags = 0;

    flags |= LABEL_FLAG_MIDDLE;

    glm::vec4 color1 = black, color2 = white;
    if (td.child_count) {
      color1 = glm::vec4(0.0, 0.5, 1.0, 1.0);
      flags |= LABEL_FLAG_CENTER;
    } else if (getNodeArray().getLabelStyle() == LABEL_DARK_BOX) {
      float node_size = 0.0f;
      offset_y -= 0.8 * (3.0 + 2.0 * node_size);
      flags |= LABEL_FLAG_CENTER;
    }
        
    labels.push_back({ pos, offset_x, offset_y, pd.label_texture, flags, color1, color2 });
  }
  
  if (labels.empty()) return;
  
  std::unique_ptr<billboard_data_s[]> data(new billboard_data_s[4 * labels.size()]);
  std::unique_ptr<unsigned int[]> indices(new unsigned int[6 * labels.size()]);
  billboard_data_s * current_data = data.get();
  unsigned int * current_index = indices.get();
  int idx = 0;

  for (auto & ld : labels) {
    auto & tp = atlas.getTexturePos(ld.texture);

    if (ld.flags & LABEL_FLAG_CENTER) ld.x -= tp.width / 2.0f;
    if (ld.flags & LABEL_FLAG_MIDDLE) ld.y -= tp.height / 2.0f;
    // else y -= tp.height;
    
    float tx1 = (float)tp.x / atlas.getWidth(), ty1 = (float)tp.y / atlas.getHeight();
    float tx2 = (float)(tp.x + tp.width) / atlas.getWidth(), ty2 = (float)(tp.y + tp.height) / atlas.getHeight();

    vbo_color_s color1 = { (unsigned char)(ld.color1.x * 255.0),
			   (unsigned char)(ld.color1.y * 255.0),
			   (unsigned char)(ld.color1.z * 255.0),
			   (unsigned char)(ld.color1.w * 255.0) };
    vbo_color_s color2 = { (unsigned char)(ld.color2.x * 255.0),
			   (unsigned char)(ld.color2.y * 255.0),
			   (unsigned char)(ld.color2.z * 255.0),
			   (unsigned char)(ld.color2.w * 255.0) };
    
    *(current_data++) = { ld.pos, glm::packHalf2x16(glm::vec2(ld.x, ld.y + tp.height)), glm::packHalf2x16(glm::vec2(tx1, ty1)), color1, color2 };
    *(current_data++) = { ld.pos, glm::packHalf2x16(glm::vec2(ld.x, ld.y)), glm::packHalf2x16(glm::vec2(tx1, ty2)), color1, color2 };
    *(current_data++) = { ld.pos, glm::packHalf2x16(glm::vec2(ld.x + tp.width, ld.y)), glm::packHalf2x16(glm::vec2(tx2, ty2)), color1, color2 };
    *(current_data++) = { ld.pos, glm::packHalf2x16(glm::vec2(ld.x + tp.width, ld.y + tp.height)), glm::packHalf2x16(glm::vec2(tx2, ty1)), color1, color2 };

    *current_index++ = idx + 0;
    *current_index++ = idx + 1;
    *current_index++ = idx + 3;
    *current_index++ = idx + 1;
    *current_index++ = idx + 2;
    *current_index++ = idx + 3;

    idx += 4;
  }
  
  vbo.setDrawType(VBO::TRIANGLES);
  vbo.uploadIndices(indices.get(), labels.size() * 6 * sizeof(unsigned int));
  vbo.upload(VBO::BILLBOARDS, data.get(), labels.size() * 4 * sizeof(billboard_data_s));
}

// Gauss-Seidel relaxation for links
void
Graph::relaxLinks() {
  // cerr << "relax: " << max_edge_weight << endl;
  double avg_edge_weight = total_edge_weight / getEdgeCount();
  // cerr << "avg edge weight = " << avg_edge_weight << endl;
  float alpha = getNodeArray().getAlpha2();
  auto & size_method = nodes->getNodeSizeMethod();
  bool flatten = nodes->doFlattenHierarchy();
  int num_nodes = nodes->size();
  vector<bool> processed_edges;
  processed_edges.resize(num_nodes * num_nodes);			
  auto end = end_edges();
  for (auto it = begin_edges(); it != end; ++it) {
    int tail = it->tail, head = it->head;
    if (flatten) {
      int l1 = 0, l2 = 0;
      bool is_visible = true;
      for (int p = node_geometry3[tail].parent_node; p != -1; p = node_geometry3[p].parent_node) {
	if (!nodes->getNodeData(p).isOpen()) {
	  is_visible = false;
	}
	l1++;
      }
      if (!is_visible) continue;
      for (int p = node_geometry3[head].parent_node; p != -1; p = node_geometry3[p].parent_node) {
	if (!nodes->getNodeData(p).isOpen()) {
	  is_visible = false;
	}
	l2++;
      }
      if (!is_visible) continue;
      while ( 1 ) {
	if (l1 > l2) {
	  tail = node_geometry3[tail].parent_node;
	  l1--;
	} else if (l2 > l1) {
	  head = node_geometry3[head].parent_node;
	  l2--;
	} else if (node_geometry3[tail].parent_node == node_geometry3[head].parent_node) {
	  
	  break;
	} else {
	  l1--; l2--;
	  tail = node_geometry3[tail].parent_node;
	  head = node_geometry3[head].parent_node;
	}
      }
      if (processed_edges[tail * num_nodes + head]) {
	continue;
      }
      processed_edges[tail * num_nodes + head] = true;
      processed_edges[head * num_nodes + tail] = true;
    }
    if (tail == head || (it->weight > -EPSILON && it->weight < EPSILON)) continue;
    auto & pd1 = nodes->getNodeData(tail), & pd2 = nodes->getNodeData(head);
    bool fixed1 = pd1.isFixed();
    bool fixed2 = pd2.isFixed();
    if (fixed1 && fixed2) continue;      
    glm::vec3 & pos1 = pd1.position, & pos2 = pd2.position;
    glm::vec3 d = pos2 - pos1;
    float l = glm::length(d);
    if (l < EPSILON) continue;
    auto & td1 = getNodeTertiaryData(tail), & td2 = getNodeTertiaryData(head);
    assert(td1.parent_node == td2.parent_node);
    // d *= getAlpha() * it->weight * link_strength * (l - link_length) / l;
    d *= alpha * fabsf(it->weight) / max_edge_weight; // / avg_edge_weight;
    
    float w1 = size_method.calculateSize(td1, total_indegree, total_outdegree, nodes->size());
    float w2 = size_method.calculateSize(td2, total_indegree, total_outdegree, nodes->size());
    float k;
    if (fixed1) {
      k = 1.0f;
    } else if (fixed2) {
      k = 0.0f;
    } else {
      k = w1 / (w1 + w2);
    }
    pos2 -= d * k;
    pos1 += d * (1 - k);
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
  auto & size_method = nodes->getNodeSizeMethod();
    
  auto end = end_visible_nodes();
  for (auto it = begin_visible_nodes(); it != end; ++it) {
    auto & pd = nodes->getNodeData(*it);
    auto & td = getNodeTertiaryData(*it);
    float size = size_method.calculateSize(td, total_indegree, total_outdegree, nodes->size());
    auto pos = pd.position;
    for (int p = td.parent_node; p != -1; p = getNodeTertiaryData(p).parent_node) {
      pos += nodes->getNodeData(p).position;
    }
    glm::vec3 tmp1 = display.project(pos);
    glm::vec3 tmp2 = display.project(pos + glm::vec3(size / 2.0f / node_scale, 0.0f, 0.0f));
    glm::vec2 pos1(tmp1.x, tmp1.y);
    glm::vec2 pos2(tmp2.x, tmp2.y);
    glm::vec2 tmp3 = pos2 - pos1;
    float diam = glm::length(tmp3);
    pos1 -= ppos;
    float d = glm::length(pos1) - diam;
    if (d <= 0 && (best_i == -1 || d < best_d)) {
      best_i = *it;
      best_d = d;
    }
  }
  return best_i;
}

void
Graph::extractLocationGraph(Graph & target_graph) {
  map<string, int> node_mapping;
  target_graph.getNodeArray().getTable().addTextColumn("name");

  // Column & sentiment = target_graph.getNodeArray().getNodeArray().getTable().addDoubleColumn("sentiment");

  const table::Column & lat_column = getNodeArray().getTable()["latitude"], & lon_column = getNodeArray().getTable()["longitude"];  
 
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
	target_graph.getNodeArray().setPositions(new_node_id, pair<glm::vec3, glm::vec3>(tmp, tmp));
	np.first = new_node_id;
      }

      // pair<float, unsigned int> & sn = node_sentiments[np.first];
      // sn.first += edge_sentiment;
      // sn.second++;
      
      NodeType type = getNodeArray().getNodeData(np.second).type;
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
	target_graph.getNodeArray().setPositions(new_node_id, pair<glm::vec3, glm::vec3>(tmp, tmp));
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
  
  target_graph.getNodeArray().setSRID(4326);
  target_graph.getNodeArray().setNodeSizeMethod(getNodeArray().getNodeSizeMethod());
  target_graph.updateAppearance();
  target_graph.setNodeVisibility(true);
  target_graph.setEdgeVisibility(true);
  target_graph.setFaceVisibility(false);
  target_graph.setLabelVisibility(true);  
}

static bool compareCameraDistance(const pair<int, float> & a, const pair<int, float> & b) {
  return a.second > b.second;
}

void
Graph::selectNodes(int input_node, int depth) {
  if (input_node == -1) {
    has_node_selection = false;
    for (int n = 0; n < nodes->size(); n++) {
      getNodeArray().node_geometry[n].flags |= NODE_SELECTED;
    }
  } else {
    has_node_selection = true;
    for (int n = 0; n < nodes->size(); n++) {
      getNodeArray().node_geometry[n].flags &= ~NODE_SELECTED;
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
	  
	  getNodeArray().node_geometry[node_id].flags |= NODE_SELECTED;
	  
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
  for (int i = 0; i < nodes->size(); i++) {
    auto & graph = getNodeArray().node_geometry[i].nested_graph;
    if (graph.get()) v.push_back(graph->getId());
  }
  return v;
}

std::vector<int>
Graph::getLocationGraphs() const {
  std::vector<int> v;
  for (int i = 0; i < nodes->size(); i++) {
    auto & graph = getNodeArray().node_geometry[i].nested_graph;
    if (graph.get() && graph->getLocation().get()) v.push_back(graph->getId());
  }
  return v;
}

void
Graph::refreshLayouts() {
  cerr << "resume after refreshLayouts\n";
  getNodeArray().resume2();
  for (auto & g : final_graphs) {
    g->getNodeArray().resume2();
  }
  for (int i = 0; i < nodes->size(); i++) {
    auto & graph = getNodeArray().node_geometry[i].nested_graph;
    if (graph.get()) graph->refreshLayouts();
  }
}

std::vector<std::shared_ptr<Graph> >
Graph::getNestedGraphs() {
  std::vector<std::shared_ptr<Graph> > v;
  for (int i = 0; i < nodes->size(); i++) {
    auto & graph = getNodeArray().node_geometry[i].nested_graph;
    if (graph.get()) {
      v.push_back(graph);
    }
  }
  return v;
}

int
Graph::getGraphNodeId(int graph_id) const {
  for (int i = 0; i < nodes->size(); i++) {
    auto & graph = getNodeArray().node_geometry[i].nested_graph;
    if (graph.get() && graph->getId() == graph_id) return i;
  }
  return -1;
}

void
Graph::createClusters() {
#if 0
  double precision = 0.000001;
  
  cerr << "creating arrays, precision = " << precision << "\n";
  
  double total_weight = 0;
  vector<unsigned long long> degrees;
  vector<unsigned int> links;
  vector<float> weights;

  cerr << "populating arrays\n";

  for (int v = 0; v < nodes->size(); v++) {
    unsigned int degree = 0;
    int edge = getNodeFirstEdge(v);
    while (edge != -1) {
      edge_data_s & ed = getEdgeAttributes(edge);
      
      int succ = getEdgeTargetNode(edge);
      assert(succ >= 0 && succ < nodes->size());

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

  cerr << "accumulation done, degrees.size() = " << degrees.size() << ", .back() = " << degrees.back() << ", nodes = " << nodes->size() << ", edges = " << getEdgeCount() << endl;

  assert(degrees.size() == nodes->size());
  assert(degrees.back() == getEdgeCount() ||
	 degrees.back() == 2 * getEdgeCount());
  
  cerr << "creating communities\n";

  ColorProvider colors(ColorProvider::CHART2);
  
  BinaryGraph binary_graph(nodes->size(), weights.size(), total_weight, degrees, links, weights);
  
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
      assert(partition.size() == nodes->size());

      glm::vec3 c1(1.0, 0.0, 0.0), c2(1.0, 1.0, 0.0);
      for (int i = 0; i < nodes->size(); i++) {
	int p = partition[i];
	// float f = float(p) / (n - 1);
	// glm::vec3 c = glm::normalize(glm::mix(c1, c2, f));
	// cerr << "assigning colors (" << i << "/" << n << ", p = " << p << ", f = " << f << ")\n";
	auto color = colors.getColorByIndex(p);
	getNodeArray().setNodeColor2(i, color);
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

unsigned int
Graph::getSuitableFinalGraphCount() const {
  if (nodes->size() >= 100 && 0) {
    return 5;
  } else {
    return 1;
  }
}

bool
Graph::updateSelection2(time_t start_time, time_t end_time, float start_sentiment, float end_sentiment) {
  if (nodes->getPersonality() != NodeArray::SOCIAL_MEDIA || !nodes->isTemporal()) {
    return false;
  }

  unsigned int count = getSuitableFinalGraphCount();
  if (final_graphs.size() != count) {
    if (!final_graphs.empty()) {
      final_graphs.front()->removeAllChildren();
    }
    final_graphs.clear();
    cerr << "CREATING FINALs!\n";
    if (count == 5) {
      auto g0 = createSimilar();
      auto g1 = createSimilar();
      auto g2 = createSimilar();
      auto g3 = createSimilar();
      auto g4 = createSimilar();
      g1->setMinSignificance(2.0f);
      g1->setMinScale(3.2f);
      g2->setMinSignificance(4.0f);
      g2->setMinScale(0.8f);
      g3->setMinSignificance(8.0f);
      g3->setMinScale(0.2f);
      g4->setMinSignificance(16.0f);
      g4->setMinScale(0.05f);
      addFinalGraph(g0);
      addFinalGraph(g1);
      addFinalGraph(g2);
      addFinalGraph(g3);
      addFinalGraph(g4);
    } else {
      auto g1 = createSimilar();
      assert(g1.get());
      addFinalGraph(g1);
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
    
    for (int j = 0; j < nodes->size(); j++) {
      auto & pd = getNodeArray().getNodeData(j);
      pd.flags = NODE_SELECTED;
      pd.label_visibility_val = 0;
    }
  }
    
  statistics.setSentimentRange(start_sentiment, end_sentiment);

  bool changed = false;
  for (unsigned int i = 0; i < final_graphs.size(); i++) {
    auto & g = final_graphs[i];
    assert(g.get());
    Graph * base_graph = final_graphs[0].get();
    assert(base_graph);
    if (g->updateData(start_time, end_time, start_sentiment, end_sentiment, *this, statistics, i == 0, base_graph)) {
      g->updateAppearance();
      g->incVersion();
      setLocationGraphValid(false);
      incVersion();
      getNodeArray().resume2();
      g->getNodeArray().resume2();
      changed = true;
    }
  }
  
  return changed;         
}

void
Graph::calculateEdgeCentrality() {
  int num_edges = getEdgeCount();
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
  getNodeArray().resume2();
}

struct label_data_s {
  enum { NODE, FACE } type;
  glm::vec3 world_pos;
  glm::vec2 screen_pos;
  float size;
  int index;
};

static bool compareSize(const label_data_s & a, const label_data_s & b) {
  return a.size > b.size;
}

bool
Graph::updateLabelVisibility(const DisplayInfo & display, bool reset) {
  vector<label_data_s> all_labels;
  auto & size_method = nodes->getNodeSizeMethod();
    
  auto end = end_visible_nodes();
  for (auto it = begin_visible_nodes(); it != end; ++it) {
    auto & pd = getNodeArray().getNodeData(*it);
    auto & td = getNodeTertiaryData(*it);
    if ((!pd.label.empty() || td.child_count) && td.age >= 0 && (display.isPointVisible(pd.position) || pd.getLabelVisibility())) {
      float size = size_method.calculateSize(td, total_indegree, total_outdegree, nodes->size());
      auto pos = pd.position;
      for (int p = td.parent_node; p != -1; p = getNodeTertiaryData(p).parent_node) {
	pos += nodes->getNodeData(p).position;
      }
      all_labels.push_back({ label_data_s::NODE, pos, glm::vec2(), size, *it });
    }
  }
    
  for (int i = 0; i < getFaceCount(); i++) {
    auto & fd = getFaceAttributes(i);
    if ((!fd.label.empty() || getDefaultSymbolId()) &&
	(display.isPointVisible(fd.centroid) || fd.isLabelVisible())) {
      glm::vec3 pos(fd.centroid.x, fd.centroid.y, 0.0f);
      all_labels.push_back({ label_data_s::FACE, pos, glm::vec2(), 1.0f, i });
    }
  }
  
  bool changed = false;
  
  if (all_labels.size() <= 10) {
    for (vector<label_data_s>::iterator it = all_labels.begin(); it != all_labels.end(); it++) {
      if (it->type == label_data_s::NODE) {
	auto & nd = getNodeArray().getNodeData(it->index);
	nd.setLabelVisibilityValue(1);
	changed |= nd.setLabelVisibility(true);
      } else {
	auto & fd = getFaceAttributes(it->index);
	fd.setLabelVisibilityValue(1);
	changed |= fd.setLabelVisibility(true);
      }
    }
  } else {
    sort(all_labels.begin(), all_labels.end(), compareSize);
  
    vector<label_data_s> drawn_labels;
    // const Rect2d & region = getContentRegion();
    
    for (auto & ld : all_labels) {
      auto tmp = display.project(ld.world_pos);
      ld.screen_pos = glm::vec2(tmp.x, tmp.y);
      auto & my_pos = ld.screen_pos;

      bool fits = true;
      
      for (auto & ld2 : drawn_labels) {
	auto & other_pos = ld2.screen_pos;
	
	float dx = fabsf(my_pos.x - other_pos.x);
	float dy = fabsf(my_pos.y - other_pos.y);
	
	if (dx < 200 && dy < 100) {
	  fits = false;
	}
      }
      if (fits) {
	drawn_labels.push_back(ld);
      }
      if (ld.type == label_data_s::NODE) {
	changed |= getNodeArray().updateLabelValues(ld.index, fits ? 1.00f : -1.00f);
      } else {
	changed |= updateFaceLabelValues(ld.index, fits ? 1.00f : -1.00f);
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
  } else if (nodes->hasSubGraphs()) {
    for (int i = 0; i < nodes->size(); i++) {
      auto & graph = getNodeArray().node_geometry[i].nested_graph;
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
  } else if (nodes->hasSubGraphs()) {
    for (int i = 0; i < nodes->size(); i++) {
      auto & graph = getNodeArray().node_geometry[i].nested_graph;
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

glm::vec3
Graph::getNodePosition(int node_id) const {
  auto & td = getNodeTertiaryData(node_id);
  glm::vec3 pos = nodes->getNodeData(node_id).position;
  for (int p = td.parent_node; p != -1; p = getNodeTertiaryData(p).parent_node) {
    pos += nodes->getNodeData(p).position;
  }
  return pos;
}
  
skey
Graph::getNodeKey(int node_id) const {
  const table::Column & source_id_column = getNodeArray().getTable()["source"];
  const table::Column & id_column = getNodeArray().getTable()["id"];
  
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
Graph::invalidateVisibleNodes() {
  final_graphs.clear();
  for (int i = 0; i < nodes->size(); i++) {
    auto & graph = getNodeArray().node_geometry[i].nested_graph;
    if (graph.get()) {
      graph->invalidateVisibleNodes();      
    }
  }
}

void
Graph::setNodeTexture(const skey & key, int texture) { 
  auto it2 = getNodeArray().getNodeCache().find(key);
  if (it2 != getNodeArray().getNodeCache().end()) {
    getNodeArray().setNodeTexture(it2->second, texture);
  }
  for (auto & g : final_graphs) {
    g->setNodeTexture(key, texture);
  }
}

std::shared_ptr<Graph>
Graph::getFinal(float scale) {
  for (int i = int(final_graphs.size()) - 1; i >= 0; i--) {
    auto & g = final_graphs[i];
    if (i == 0 || (scale > 0.0f && g->getEdgeCount() > 0 && scale < g->getMinScale())) {
      // cerr << "final for scale " << scale << " of graph " << getId() << ": " << (i + 1) << " / " << final_graphs.size() << " (min_scale = " << g->getMinScale() << ", min_sig = " << g->getMinSignificance() << ", edges = " << g->getEdgeCount() << ")\n";
      return g;
    }
  }
  return std::shared_ptr<Graph>(0);
}

const std::shared_ptr<const Graph>
Graph::getFinal(float scale) const {
  for (int i = int(final_graphs.size()) - 1; i >= 0; i--) {
    auto & g = final_graphs[i];
    if (i == 0 || (scale > 0.0f && g->getEdgeCount() > 0 && scale < g->getMinScale())) {
      // cerr << "final for scale " << scale << " of graph " << getId() << ": " << (i + 1) << " / " << final_graphs.size() << " (min_scale = " << g->getMinScale() << ", min_sig = " << g->getMinSignificance() << ", edges = " << g->getEdgeCount() << ")\n";
      return g;
    }
  }
  return std::shared_ptr<Graph>(0);
}

void
Graph::updateAppearance() {
  if (node_geometry3.size() < nodes->size()) node_geometry3.resize(nodes->size());
#if 0
  auto & method = getNodeArray().getNodeSizeMethod();
  if (method.getValue() == SizeMethod::SIZE_FROM_COLUMN) {
    table::Column & sc = getNodeArray().getTable()[method.getColumn()];
    for (unsigned int i = 0; i < nodes->size(); i++) {
      node_geometry3[i].size = 2 * (1 + log(1 + sc.getDouble(i)) / log(1.5));
    }
    version++;
  } else if (method.getValue() == SizeMethod::SIZE_FROM_NODE_COUNT) {
    for (unsigned int i = 0; i < nodes->size(); i++) {
      auto & nested_graph = getNodeArray().node_geometry[i].nested_graph;
      float a = nested_graph.get() ? nested_graph->nodes->size() : 0;
      node_geometry3[i].size = 2 * (1 + log(1 + a) / log(2));
    }
    version++;
  } else if (method.getValue() == SizeMethod::CONSTANT) {
    for (unsigned int i = 0; i < nodes->size(); i++) {
      node_geometry3[i].size = method.getConstant();
    }    
    version++;
  }
#endif

  auto & label_method = nodes->getLabelMethod();
  if (label_method.getValue() != LabelMethod::FIXED_LABEL) {
    for (int i = 0; i < getFaceCount(); i++) {
      string label, name, text, id;
      if (label_method.getValue() == LabelMethod::LABEL_FROM_COLUMN) {
	label = getFaceData()[label_method.getColumn()].getText(i);
      } else {
	for (auto & cd : getFaceData().getColumns()) {
	  string n = StringUtils::toLower(cd.first);
	  if (n == "label") {
	    label = cd.second->getText(i);
	  } else if (n == "name") {
	    name = cd.second->getText(i);
	  } else if (n == "text") {
	    text = cd.second->getText(i);
	  } else if (n == "id") {
	    id = cd.second->getText(i);
	  }
	}
	if (label_method.getValue() == LabelMethod::AUTOMATIC_LABEL && !label.empty()) {
	  if (!name.empty()) {
	    label = name;
	  } else if (!text.empty()) {
	    label = text;
	  } else if (!id.empty()) {
	    label = id;
	  }    
	}
      }
      if (!label.empty()) cerr << "setting label for face " << i << ": " << label << endl;
      getFaceAttributes(i).setLabel(label);
    }
  }
  
  nodes->updateAppearance();
}

static inline void applyGravityToNode(float k, node_data_s & pd, const node_tertiary_data_s & td, float weight) {
  if (!pd.isFixed()) {
    const glm::vec3 & pos = pd.position;
    float d = glm::length(pos);
    if (d > 0.001) {
      pd.position -= pos * (k * sqrtf(d) / d * weight);
    }
  }
}

void
Graph::applyGravity(float gravity) {
  float k = nodes->getAlpha2() * gravity;
  if (k > EPSILON) {
    vector<bool> processed_nodes;
    processed_nodes.resize(nodes->size());
    list<int> parent_nodes;

    auto end = end_edges();
    for (auto it = begin_edges(); it != end; ++it) {
      if (!processed_nodes[it->tail]) {
	processed_nodes[it->tail] = true;
	auto & pd = nodes->getNodeData(it->tail);
	auto & td = getNodeTertiaryData(it->tail);
	float factor = 1.0f;
	if (td.parent_node >= 0) {
	  parent_nodes.push_back(td.parent_node);
	  factor = 96.0f;
	}
	applyGravityToNode(factor * k, pd, td, nodes->hasTemporalCoverage() ? td.coverage_weight : 1.0f);
      }
      if (!processed_nodes[it->head]) {
	processed_nodes[it->head] = true;
	auto & pd = nodes->getNodeData(it->head);
	auto & td = getNodeTertiaryData(it->head);
	float factor = 1.0f;
	if (td.parent_node >= 0) {
	  parent_nodes.push_back(td.parent_node);
	  factor = 96.0f;
	}	
	applyGravityToNode(factor * k, pd, td, nodes->hasTemporalCoverage() ? td.coverage_weight : 1.0f);
      }
    }
    while (!parent_nodes.empty()) {
      int n = parent_nodes.front();
      parent_nodes.pop_front();
      if (!processed_nodes[n]) {
	processed_nodes[n] = true;
	auto & pd = nodes->getNodeData(n);
	auto & td = getNodeTertiaryData(n);
	float factor = 1.0f;
	if (td.parent_node >= 0) {
	  parent_nodes.push_back(td.parent_node);
	  factor = 50.0f;	 
	}
	applyGravityToNode(factor * k, pd, td, nodes->hasTemporalCoverage() ? td.coverage_weight : 1.0f);
      }
    }
  }
}

void
Graph::applyDragAndAge(RenderMode mode, float friction) {
  auto end = end_visible_nodes();
  for (auto it = begin_visible_nodes(); it != end; ++it) {
    auto & pd = nodes->getNodeData(*it);
    auto & td = node_geometry3[*it];
      
    glm::vec3 & pos = pd.position, & ppos = pd.prev_position;
    
    glm::vec3 new_pos = pos - (ppos - pos) * friction;
    if (mode == RENDERMODE_2D) {
      new_pos.z = 0;
    }
    pd.prev_position = pos;
    pd.position = new_pos;
    td.age += 1.0f / 50.0f;
  }
  
  version++;
}

bool
Graph::updateData(time_t start_time, time_t end_time, float start_sentiment, float end_sentiment, Graph & source_graph, RawStatistics & stats, bool is_first_level, Graph * base_graph) {
  assert(0);
  return false;
}

bool
Graph::isNodeVisible(int node) const {
  if (node >= node_geometry3.size()) {
    return false;
  } else {
    // PROBLEM: node might not be visible even with children, since the children might be invisible
    return node_geometry3[node].first_edge != -1 || node_geometry3[node].child_count > 0 || node_geometry3[node].indegree > 0;
  }
}

int
Graph::addEdge(int n1, int n2, int face, float weight, int arc, long long coverage) {
  assert(n1 != -1 && n2 != -1);
  int edge = (int)edge_attributes.size();

  if (!isNodeVisible(n1)) {
    setNodeAge(n1, initial_node_age); // this is first edge
  }
  int next_node_edge = getNodeFirstEdge(n1);
  
  setNodeFirstEdge(n1, edge);

  if (n1 != n2) {
    if (!isNodeVisible(n2)) {
      setNodeAge(n2, initial_node_age); // this is first edge
    }
    updateOutdegree(n1, 1.0f); // weight);
    updateIndegree(n2, 1.0f); // weight);
  }
  updateNodeCoverage(n1, coverage);
  updateNodeCoverage(n2, coverage);
  
  edge_attributes.push_back(edge_data_s( weight, n1, n2, next_node_edge, -1, -1, arc, coverage ));
  total_edge_weight += fabsf(weight);
  if (weight > max_edge_weight) max_edge_weight = weight;

  if (face != -1) {
    setEdgeFace(edge, face);
  }
  
  incVersion();
  return edge;
}

void
Graph::addChild(int parent, int child) {
  assert(parent != child);
  if (node_geometry3.size() <= parent) node_geometry3.resize(parent + 1);
  if (node_geometry3.size() <= child) node_geometry3.resize(child + 1);

  if (!isNodeVisible(parent)) {
    // PROBLEM: parent doesn't actually become visible, if the added child has no edges or children with edges
    setNodeAge(parent, initial_node_age);
  }
  
  assert(node_geometry3[child].parent_node == -1);
  assert(node_geometry3[child].next_child == -1);
  node_geometry3[child].next_child = node_geometry3[parent].first_child;
  node_geometry3[parent].first_child = child;
  node_geometry3[child].parent_node = parent;
  node_geometry3[parent].child_count++;
  nodes->getNodeData(parent).label_texture = 0;
  node_geometry3[parent].coverage = 0xffffffffffffffffULL;
  node_geometry3[parent].coverage_weight = 1.0f;
  nodes->getNodeData(child).position -= nodes->getNodeData(parent).position;
  version++;
  assert(node_geometry3[child].parent_node == parent);
  cerr << "added child " << child << " to " << parent << endl;
}

void
Graph::removeChild(int child) {
  if (node_geometry3.size() <= child) node_geometry3.resize(child + 1);
  int parent = node_geometry3[child].parent_node;
  assert(parent != -1);
  if (parent != -1) {
    if (node_geometry3[parent].first_child == child) {
      node_geometry3[parent].first_child = node_geometry3[child].next_child;
    } else {
      int n = node_geometry3[parent].first_child;
      while (n != -1) {
	int next_child = node_geometry3[n].next_child;
	if (next_child == child) {
	  node_geometry3[n].next_child = node_geometry3[child].next_child;
	  break;
	}
	n = next_child;
	assert(n != -1);
      }
    }
    node_geometry3[child].parent_node = node_geometry3[child].next_child = -1;
    node_geometry3[parent].child_count--;
    nodes->getNodeData(parent).label_texture = 0;
    nodes->getNodeData(child).position += nodes->getNodeData(parent).position;
    version++;
  }        
}

void
Graph::removeAllChildren() {
  for (int i = 0; i < nodes->size(); i++) {
    auto & nd = nodes->getNodeData(i);
    auto & td = node_geometry3[i];
    if (td.child_count) {
      td.child_count = 0;
      td.first_child = -1;
    }
    if (td.parent_node != -1) {
      nd.position = getNodePosition(i);
      td.parent_node = -1;
    }
  }
}
