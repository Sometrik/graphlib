#ifndef _POINTCLOUD_H_
#define _POINTCLOUD_H_

#include <Graph.h>

class PointCloud : public Graph {
 public:
  PointCloud(int _id = 0);
  PointCloud(const PointCloud & other);
  
  std::shared_ptr<Graph> createSimilar() const override;
  Graph * copy() const override { return new PointCloud(*this); }
};

#endif
