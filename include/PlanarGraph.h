#ifndef _PLANARGRAPH_H_
#define _PLANARGRAPH_H_

#include <Graph.h>

struct planar_edge_data_s : public edge_data_s {
 planar_edge_data_s() { }
 planar_edge_data_s(float _weight, int _tail, int _head, int _next_node_edge, int _face, int _next_face_edge, int _arc)
   : edge_data_s(_weight, _tail, _head, _next_node_edge, _face, _next_face_edge, _arc) { }

 int pair_edge = -1;
};

class PlanarGraph : public Graph {
 public:
  PlanarGraph(int _id = 0);
  PlanarGraph(const PlanarGraph & other);
  
  bool checkConsistency() const override;

  std::shared_ptr<Graph> createSimilar() const override;
  Graph * copy() const override { return new PlanarGraph(*this); }
  
  void addUniversalRegion() override;

  int addEdge(int n1, int n2, int face = -1, float weight = 1.0f, int arc = 0) override {
    int edge = (int)edge_attributes.size();
    int next_node_edge = -1;
    if (n1 != -1) {
      next_node_edge = getNodeFirstEdge(n1);
      setNodeFirstEdge(n1, edge);
      updateOutdegree(n1, 1);
      updateNodeSize(n1);
    }
    if (n2 != -1 && n1 != n2) {
      updateIndegree(n2, 1);
      updateNodeSize(n2);
    }

    edge_attributes.push_back(planar_edge_data_s( weight, n1, n2, next_node_edge, -1, -1, arc ));
    edges.addRow();
    total_edge_weight += fabsf(weight);

    setEdgeFace(edge, face);

    return edge;
  }

  int addRegion() override {
    int region_id = (int)region_attributes.size();
    region_attributes.push_back({ { 255, 255, 255, 255 }, "", Rect2d(), -1 });
    regions.addRow();
    return region_id;
  }

  int getRegionFirstFace(int region) const override {
    return region_attributes[region].first_face;
  }

  int getRegionNextFace(int region, int face) const override {
    return face_attributes[face].next_face_in_region;
  }

  int getNextFaceEdge(int edge) const override {
    return edge_attributes[edge].next_face_edge;
  }

  std::set<int> getAdjacentRegions() const override;

  void updateMBR(int edge) override;

  void mapFacesToNodes(Graph & target) override;
  void mapRegionsToNodes(Graph & target) override;
  void calculateRegionAreas() override;
  void colorizeRegions() override;

  int findContainingRegion(const glm::dvec3 & v) const;
  void spatialAggregation(const Graph & other) override;

  void clear() override {
    Graph::clear();
    edge_attributes.clear();
  }
  
 protected:
  edge_data_s & getEdgeAttributes(int i) override { return edge_attributes[i]; }
  const edge_data_s & getEdgeAttributes(int i) const override { return edge_attributes[i]; }
  EdgeIterator begin_edges() override { return EdgeIterator(&(edge_attributes.front()), sizeof(planar_edge_data_s)); }
  EdgeIterator end_edges() override {
    EdgeIterator it(&(edge_attributes.back()), sizeof(planar_edge_data_s));
    ++it;
    return it;
  }
  ConstEdgeIterator begin_edges() const override { return ConstEdgeIterator(&(edge_attributes.front()), sizeof(planar_edge_data_s)); }
  ConstEdgeIterator end_edges() const override {
    ConstEdgeIterator it(&(edge_attributes.back()), sizeof(planar_edge_data_s));
    ++it;
    return it;
  }
  
 private:
  std::vector<planar_edge_data_s> edge_attributes;
};

#endif
