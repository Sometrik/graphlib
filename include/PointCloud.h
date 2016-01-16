#ifndef _POINTCLOUD_H_
#define _POINTCLOUD_H_

#include <Graph.h>

class PointCloud : public Graph {
 public:
  PointCloud(int _id = 0);
  PointCloud(const PointCloud & other);
  
  std::shared_ptr<Graph> createSimilar() const override;
  Graph * copy() const override { return new PointCloud(*this); }
      
  int addEdge(int n1, int n2, int face = -1, float weight = 1.0f, int arc = 0) override {
    assert(0);
    return -1;
  }

 protected:
  edge_data_s & getEdgeAttributes(int i) override { return null_edge; }
  const edge_data_s & getEdgeAttributes(int i) const override { return null_edge; }

  EdgeIterator begin_edges() override { return EdgeIterator(0, 0); }
  EdgeIterator end_edges() override {  return EdgeIterator(0, 0); }
  ConstEdgeIterator begin_edges() const override { return ConstEdgeIterator(0, 0); }
  ConstEdgeIterator end_edges() const override {  return ConstEdgeIterator(0, 0); }

 private:
  edge_data_s null_edge;
};

#endif
