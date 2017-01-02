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

  std::set<int> getAdjacentRegions() const override;

  void mapFacesToNodes(Graph & target) override;
  void mapRegionsToNodes(Graph & target) override;
  void calculateRegionAreas() override;
  void colorizeRegions() override;

  int findContainingRegion(const glm::dvec3 & v) const;
  void spatialAggregation(const Graph & other) override;
};

#endif
