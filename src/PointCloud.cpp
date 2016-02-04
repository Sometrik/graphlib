#include "PointCloud.h"

using namespace std;

PointCloud::PointCloud(int _id) : Graph(0, _id) {  

}

PointCloud::PointCloud(const PointCloud & other)
  : Graph(other) {

}

std::shared_ptr<Graph>
PointCloud::createSimilar() const {
  std::shared_ptr<Graph> graph(new PointCloud(getId()));
  graph->setLocationGraphValid(false);
  graph->setTemporal(isTemporal());
  graph->setPersonality(getPersonality());
  graph->setHasTextures(hasTextures());

  return graph;
}

