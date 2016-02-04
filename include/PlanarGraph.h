#ifndef _PLANARGRAPH_H_
#define _PLANARGRAPH_H_

#include <Graph.h>

class PlanarGraph : public Graph {
 public:
  PlanarGraph(int _id = 0);
  PlanarGraph(const PlanarGraph & other);
  
  bool checkConsistency() const override;

  std::shared_ptr<Graph> createSimilar() const override;
  Graph * copy() const override { return new PlanarGraph(*this); }
  
#if 0
  void addUniversalRegion() override;

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
#endif

  std::set<int> getAdjacentRegions() const override;

  void updateMBR(int edge) override;

  void mapFacesToNodes(Graph & target) override;
  void mapRegionsToNodes(Graph & target) override;
  void calculateRegionAreas() override;
  void colorizeRegions() override;

  int findContainingRegion(const glm::dvec3 & v) const;
  void spatialAggregation(const Graph & other) override;
};

#endif
