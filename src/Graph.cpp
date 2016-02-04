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

#define EPSILON 0.0000000001

using namespace std;

int Graph::next_id = 1;

Graph::Graph(int _dimensions, int _id) : dimensions(_dimensions),
					 id(_id)
{
  if (!id) id = next_id++;
}


Graph::~Graph() {
  
}

Graph::Graph(const Graph & other)
  : nodes(other.nodes), // copy node array by reference
    // edges(other.edges),
    faces(other.faces),
    face_attributes(other.face_attributes),
    edge_attributes(other.edge_attributes),
    node_color_column(other.node_color_column),
    version(other.version),
    dimensions(other.dimensions),
    flags(other.flags),
    source_id(other.source_id),
    personality(other.personality)
{
  id = next_id++;
#if 0
  for (auto & d : node_geometry) {
    if (d.nested_graph.get()) {
      d.nested_graph = std::shared_ptr<Graph>(d.nested_graph->copy());
    }
  }
#endif
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
#endif

void
Graph::randomizeGeometry(bool use_2d) {
  assert(!hasSpatialData());
  mbr = Rect2d();
  unsigned int num_nodes = getNodeArray().size();
  for (unsigned int i = 0; i < num_nodes; i++) {
    glm::vec3 v( 100.0 * ((float)rand() / RAND_MAX) - 50,
		 100.0 * ((float)rand() / RAND_MAX) - 50,
		 (use_2d ? 0 : 100.0 * ((float)rand() / RAND_MAX) - 50)
		 );
    getNodeArray().node_geometry[i].position = getNodeArray().node_geometry[i].prev_position = v;
    mbr.growToContain(v.x, v.y);

    auto & graph = getNodeArray().node_geometry[i].nested_graph;
    if (graph.get() && !graph->hasSpatialData()) {
      graph->randomizeGeometry(use_2d);       
    }
  }
}

void
Graph::createRegionVBO(VBO & vbo, bool spherical, float earth_radius) const {
  if (!getFaceCount()) {
    return;
  }
  
  vector<arc_data_2d_s> new_geometry;
  vector<unsigned int> front_indices, back_indices;

  glm::vec3 normal(0.0f);

  if (hasArcData()) {
    cerr << "creating complex region VBO, faces = " << getFaceCount() << "\n";
    for (unsigned int face = 0; face < getFaceCount(); face++) {   
      auto this_color = getFaceColor(face);
      glm::vec4 color( this_color.r * 255.0f, this_color.g * 255.0f, this_color.b * 255.0f, this_color.a * 255.0f );
      if (!color.w || 1) color = nodes->getDefaultFaceColor();
      if (!color.w) color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
      
      list<list<int> > part_arcs;
      // part_arcs.push_back(list<int>());
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
	  int start_index = new_geometry.size();
	  int vi = 0;
	  for (auto & arc_id : arcs) {
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
    if (!color.w || 1) color = nodes->getDefaultEdgeColor();
    if (!color.w) color = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
#else
    glm::vec4 color = nodes->getDefaultEdgeColor();
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

    cerr << "uploading, vbo = " << &vbo << ", edges = " << getEdgeCount() << ", in = " << in << ", vn = " << vn << "\n";
    
    vbo.upload(VBO::ARCS_2D, new_geometry.get(), vn * sizeof(arc_data_2d_s));
    vbo.uploadIndices(indices.get(), in * sizeof(unsigned int));
  } else {
    unsigned int ec = getEdgeCount();
    unsigned int asize = 2 * ec * sizeof(line_data_s);
    std::unique_ptr<line_data_s[]> new_geometry(new line_data_s[2 * ec]);
    // vector<unsigned int> node_mapping, indices;
    // node_mapping.resize(nodes->size());
    graph_color_s sel_color = { 100, 200, 255, 255 };
    unsigned int vn = 0;
    auto end = end_edges();
    for (auto it = begin_edges(); it != end; ++it) {    
      if (it->tail == it->head) continue;
      auto & g1 = nodes->node_geometry[it->tail], & g2 = nodes->node_geometry[it->head];
      // int i1 = node_mapping[it->tail], i2 = node_mapping[it->head];
      bool edge_selected = (g1.flags & NODE_SELECTED) || (g2.flags & NODE_SELECTED);
      if (1) { // !i1) {
	glm::vec3 pos;
	if (is_spherical) {
	  double lat = g1.position.y / 180.0 * M_PI, lon = g1.position.x / 180.0 * M_PI;
	  pos = glm::vec3( -earth_radius * cos(lat) * cos(lon),
			   earth_radius * sin(lat),
			   earth_radius * cos(lat) * sin(lon)
			   );
	} else {
	  pos = g1.position;
	}
	auto color = edge_selected ? sel_color : g1.color;
	float a = powf(it->weight / max_edge_weight, 0.9);
	line_data_s s = { int((255 * (1-a)) + color.r * a), int((255 * (1-a)) + color.g * a), int((255 * (1-a)) + color.b * a), int((255 * (1-a)) + color.a * a), pos, g1.age, 1.0f }; // g1.size
	// i1 = node_mapping[it->tail] = vn + 1;
	*((line_data_s*)(new_geometry.get()) + vn) = s;      
	vn++;
      }
      if (1) { // !i2) {
	glm::vec3 pos;
	if (is_spherical) {
	  double lat = g2.position.y / 180.0 * M_PI, lon = g2.position.x / 180.0 * M_PI;
	  pos = glm::vec3( -earth_radius * cos(lat) * cos(lon),
			   earth_radius * sin(lat),
			   earth_radius * cos(lat) * sin(lon)
			   );
	} else {
	  pos = g2.position;
	}
	auto color = edge_selected ? sel_color : g2.color;
	float a = powf(it->weight / max_edge_weight, 0.9);
	// i2 = node_mapping[it->head] = vn + 1;
	line_data_s s = { int((255 * (1-a)) + color.r * a), int((255 * (1-a)) + color.g * a), int((255 * (1-a)) + color.b * a), int((255 * (1-a)) + color.a * a), pos, g2.age, 1.0f }; // g.size
	*((line_data_s*)(new_geometry.get()) + vn) = s;
	vn++;
      }
      // indices.push_back(i1 - 1);
      // indices.push_back(i2 - 1);      
    }
    assert(vn <= 2 * ec);
    // cerr << "uploading edges: vn = " << vn << ", indices = " << indices.size() << endl;
    vbo.upload(VBO::EDGES, new_geometry.get(), vn * sizeof(line_data_s));
    // vbo.uploadIndices(&(indices.front()), indices.size() * sizeof(unsigned int));
  }
}

void
Graph::createNodeVBOForSprites(VBO & vbo, bool is_spherical, float earth_radius) const {
  if (!getEdgeCount()) {
    return;
  }

  vector<bool> stored_nodes;
  stored_nodes.resize(nodes->size());
  vector<node_vbo_s> new_geometry;
  unsigned int edge_count = 0;

  auto end = end_edges();
  for (auto it = begin_edges(); it != end; ++it) {
    assert(it->tail >= 0 && it->tail < nodes->size());
    assert(it->head >= 0 && it->head < nodes->size());
    if (!stored_nodes[it->tail]) {
      stored_nodes[it->tail] = true;
      auto & nd = nodes->getNodeData(it->tail);
      auto & td = getNodeTertiaryData(it->tail);
      new_geometry.push_back({ nd.color.r, nd.color.g, nd.color.b, nd.color.a, nd.normal, nd.position, nd.age, td.size, nd.texture, nd.flags });
    }
    if (!stored_nodes[it->head]) {
      stored_nodes[it->head] = true;
      auto & nd = nodes->getNodeData(it->head);
      auto & td = getNodeTertiaryData(it->head);
      new_geometry.push_back({ nd.color.r, nd.color.g, nd.color.b, nd.color.a, nd.normal, nd.position, nd.age, td.size, nd.texture, nd.flags });
    }
    edge_count++;
  }
  
  // cerr << "uploaded node vbo: edges = " << edge_count << ", nodes = " << new_geometry.size() << endl;
  
  vbo.upload(VBO::NODES, &(new_geometry.front()), new_geometry.size() * sizeof(node_vbo_s));
}

void
Graph::createNodeVBOForQuads(VBO & vbo, const TextureAtlas & atlas, float node_scale, bool is_spherical, float earth_radius) const {
  if (!nodes->size()) {
    return;
  }

  assert(0);

  std::unique_ptr<vbo_data2_s[]> data(new vbo_data2_s[4 * nodes->size()]);
  std::unique_ptr<unsigned int[]> indices(new unsigned int[6 * nodes->size()]);
  
  vbo_data2_s * current_data = data.get();
  unsigned int * current_index = indices.get();

  glm::vec3 normal(1.0f, 0.0f, 0.0f);
  int idx = 0;

  for (int i = 0; i < nodes->size(); i++) { 
    auto & pd = getNodeArray().getNodeData(i);
    auto & td = getNodeTertiaryData(i);
    
    int texture = pd.texture;
    float size = td.size / node_scale;
    
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
  vbo.uploadIndices(indices.get(), nodes->size() * 6 * sizeof(unsigned int));
  vbo.upload(VBO::T2F_C4F_N3F_V3F, data.get(), nodes->size() * 4 * sizeof(vbo_data2_s));
}

void
Graph::createLabelVBO(VBO & vbo, const TextureAtlas & atlas, float node_scale) const {
  unsigned int label_count = 0;
  for (unsigned int i = 0; i < nodes->size(); i++) {
    auto & data = getNodeArray().getNodeData(i);
    auto & pd = getNodeArray().getNodeData(i);
    if (data.flags & NODE_LABEL_VISIBLE && pd.label_texture) label_count++;
  }

  // cerr << "creating label vbo (n = " << label_count << ")\n";
  
  if (!label_count) return;
  
  std::unique_ptr<billboard_data_s[]> data(new billboard_data_s[4 * label_count]);
  std::unique_ptr<unsigned int[]> indices(new unsigned int[6 * label_count]);

  billboard_data_s * current_data = data.get();
  unsigned int * current_index = indices.get();

  int idx = 0;
  const table::Column & user_type = getNodeArray().getTable()["type"];
  unsigned int actual_label_count = 0;
  
  for (unsigned int i = 0; i < nodes->size(); i++) {
    auto & pd = getNodeArray().getNodeData(i);
    // auto & sd = getNodeArray().getNodeSecondaryData(i);
    if (!(pd.flags & NODE_LABEL_VISIBLE && pd.label_texture)) continue;
    
    auto & pos = getNodeArray().getPosition(i);
    // unsigned int flags = 0;
    int offset_x = 0, offset_y = 0;
    bool text_center = false;
    bool text_middle = true;

    if (getNodeArray().getLabelStyle() == LABEL_DARK_BOX) {
      float node_size = 0.0f;
      offset_y -= 0.8 * (3.0 + 2.0 * node_size);
      text_center = true;
    }
        
    auto & tp = atlas.getTexturePos(pd.label_texture);

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
  cerr << "relax: " << max_edge_weight << endl;
  double avg_edge_weight = total_edge_weight / getEdgeCount();
  // cerr << "avg edge weight = " << avg_edge_weight << endl;
  float alpha = getNodeArray().getAlpha2();
  auto end = end_edges();
  for (auto it = begin_edges(); it != end; ++it) {
    int s = it->tail, t = it->head;
    if (s == t || (it->weight > -EPSILON && it->weight < EPSILON)) continue;
    auto & pd1 = nodes->getNodeData(s), & pd2 = nodes->getNodeData(t);
    bool fixed1 = pd1.flags & NODE_FIXED_POSITION;
    bool fixed2 = pd2.flags & NODE_FIXED_POSITION;
    if (fixed1 && fixed2) continue;      
    glm::vec3 & pos1 = pd1.position, & pos2 = pd2.position;
    glm::vec3 d = pos2 - pos1;
    float l = glm::length(d);
    if (l < EPSILON) continue;
    if (pd1.cluster_id != pd2.cluster_id && 0) continue;
    auto & td1 = getNodeTertiaryData(s), & td2 = getNodeTertiaryData(t);
    // d *= getAlpha() * it->weight * link_strength * (l - link_length) / l;
    d *= alpha * fabsf(it->weight) / max_edge_weight; // / avg_edge_weight;
    float w1 = td1.size; // * td1.coverage_weight;
    float w2 = td2.size; // * td2.coverage_weight;
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
  for (int i = 0; i < nodes->size(); i++) {
    auto & pd = nodes->getNodeData(i);
    auto & td = getNodeTertiaryData(i);
    glm::vec3 tmp1 = display.project(pd.position);
    glm::vec3 tmp2 = display.project(pd.position + glm::vec3(td.size / 2.0f / node_scale, 0.0f, 0.0f));
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
  target_graph.getNodeArray().setNodeVisibility(true);
  target_graph.getNodeArray().setEdgeVisibility(true);
  target_graph.getNodeArray().setFaceVisibility(false);
  // target_graph.getNodeArray().setRegionVisibility(false);
  target_graph.getNodeArray().setLabelVisibility(true);  
}

static bool compareCameraDistance(const pair<int, float> & a, const pair<int, float> & b) {
  return a.second > b.second;
}

vector<int>
Graph::createSortedNodeIndices(const glm::vec3 & camera_pos) const {
  // need to iterate over edges
  assert(0);
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

void
Graph::simplifyWithClusters(const std::vector<int> & clusters, Graph & target_graph) {
  map<int, int> created_clusters;
  map<int, map<int, int> > seen_edges;
  
  for (int n1 = 0; n1 < nodes->size(); n1++) {
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
#ifndef _WIN32
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
  if (getPersonality() != Graph::SOCIAL_MEDIA || !isTemporal()) {
    return false;
  }

  unsigned int count = getSuitableFinalGraphCount();
  if (final_graphs.size() != count) {
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
      pd.age = -2.0f;
      pd.flags = NODE_SELECTED;
      pd.label_visibility_val = 0;
    }
  }
    
  statistics.setSentimentRange(start_sentiment, end_sentiment);

  bool changed = false;
  for (unsigned int i = 0; i < final_graphs.size(); i++) {
    auto & g = final_graphs[i];
    Graph * base_graph = final_graphs[0].get();
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
  vector<bool> processed_nodes;
  processed_nodes.resize(nodes->size());

  auto end = end_edges();
  for (auto it = begin_edges(); it != end; ++it) {
    if (!processed_nodes[it->tail]) {
      processed_nodes[it->tail] = true;      
      auto & pd = getNodeArray().getNodeData(it->tail);
      auto & td = getNodeTertiaryData(it->tail);
      if (!pd.label.empty() || getNodeArray().getDefaultSymbolId()) {
	if (pd.age >= 0 && (display.isPointVisible(pd.position) ||
			    getNodeArray().getNodeLabelVisibility(it->tail))) {
	  glm::vec3 win = display.project(pd.position);
	  bool is_selected = false; // graph.getId() == selected_node.first && i == selected_node.second;
	  all_labels.push_back({ glm::vec2(win.x, win.y), td.size, it->tail, is_selected });
	}
      }
    }

    if (!processed_nodes[it->head]) {
      processed_nodes[it->head] = true;     
      auto & pd = getNodeArray().getNodeData(it->head);
      auto & td = getNodeTertiaryData(it->head);
      if (!pd.label.empty() || getNodeArray().getDefaultSymbolId()) {
	if (pd.age >= 0 && (display.isPointVisible(pd.position) ||
			    getNodeArray().getNodeLabelVisibility(it->head))) {
	  glm::vec3 win = display.project(pd.position);
	  bool is_selected = false; // graph.getId() == selected_node.first && i == selected_node.second;
	  all_labels.push_back({ glm::vec2(win.x, win.y), td.size, it->head, is_selected });
	}
      }
    }
  }
  
  bool changed = false;
  
  if (all_labels.size() <= 10) {
    for (vector<label_data_s>::iterator it = all_labels.begin(); it != all_labels.end(); it++) {
      getNodeArray().setNodeLabelVisibilityValue(it->index, 1);
      changed |= getNodeArray().setNodeLabelVisibility(it->index, true);
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
	getNodeArray().setNodeLabelVisibilityValue(it->index, fits ? 1 : 0);
	changed |= getNodeArray().setNodeLabelVisibility(it->index, fits);
      } else {
	changed |= getNodeArray().updateNodeLabelValues(it->index, fits ? 1.00f : -1.00f);
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
  } else if (hasSubGraphs()) {
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

void
Graph::setLabelTexture(const skey & key, int texture) {
  auto it2 = getNodeArray().getNodeCache().find(key);
  if (it2 != getNodeArray().getNodeCache().end()) {
    getNodeArray().setLabelTexture(it2->second, texture);
  }
  for (auto & g : final_graphs) {
    g->setLabelTexture(key, texture);
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
  }
}

static inline void applyGravityToNode(float k, node_data_s & pd, const node_tertiary_data_s & td) {
  if (!(pd.flags & NODE_FIXED_POSITION)) {
    glm::vec3 origin, pos = pd.position;
#if 0
    if (pd.cluster_id >= 0) origin = getClusterAttributes(pd.cluster_id).position;
#endif
    pos -= origin;
    float d = glm::length(pos);
    if (d > 0.001) {
      pos -= pos * (k * sqrtf(d) / d) * td.coverage_weight;
      pos += origin;
      pd.position = pos;
    }
  }
}

void
Graph::applyGravity(float gravity) {
  float k = nodes->getAlpha2() * gravity;
  if (k > EPSILON) {
    vector<bool> processed_nodes;
    processed_nodes.resize(nodes->size());
    
    auto end = end_edges();
    for (auto it = begin_edges(); it != end; ++it) {
      if (!processed_nodes[it->tail]) {
	processed_nodes[it->tail] = true;
	applyGravityToNode(k, nodes->getNodeData(it->tail), getNodeTertiaryData(it->tail));
      }
      if (!processed_nodes[it->head]) {
	processed_nodes[it->head] = true;
	applyGravityToNode(k, nodes->getNodeData(it->head), getNodeTertiaryData(it->head));
      }
    }

#if 0
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
#endif
  }
}

static inline void applyDragAndAgeToNode(RenderMode mode, float friction, node_data_s & pd) {
  glm::vec3 & pos = pd.position, & ppos = pd.prev_position;
    
  glm::vec3 new_pos = pos - (ppos - pos) * friction;
  if (mode == RENDERMODE_2D) {
    new_pos.z = 0;
  }
  pd.prev_position = pos;
  pd.position = new_pos;
  pd.age += 1.0f / 50.0f;
}

void
Graph::applyDragAndAge(RenderMode mode, float friction) {
  vector<bool> processed_nodes;
  processed_nodes.resize(nodes->size());
  
  auto end = end_edges();
  for (auto it = begin_edges(); it != end; ++it) {
    if (!processed_nodes[it->tail]) {
      processed_nodes[it->tail] = true;
      applyDragAndAgeToNode(mode, friction, nodes->getNodeData(it->tail));
    }
    if (!processed_nodes[it->head]) {
      processed_nodes[it->head] = true;
      applyDragAndAgeToNode(mode, friction, nodes->getNodeData(it->head));
    }
  }
  
#if 0
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
#endif
  version++;
}

bool
Graph::updateData(time_t start_time, time_t end_time, float start_sentiment, float end_sentiment, Graph & source_graph, RawStatistics & stats, bool is_first_level, Graph * base_graph) {
  assert(0);
  return false;
}

int
Graph::addEdge(int n1, int n2, int face, float weight, int arc, long long coverage) {
  assert(n1 != -1 && n2 != -1);
  int edge = (int)edge_attributes.size();

  int next_node_edge = getNodeFirstEdge(n1);
  setNodeFirstEdge(n1, edge);

  if (n1 != n2) {
    updateOutdegree(n1, 1.0f); // weight);
    updateIndegree(n2, 1.0f); // weight);
  }
  updateNodeSize(n1);
  updateNodeSize(n2);
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
