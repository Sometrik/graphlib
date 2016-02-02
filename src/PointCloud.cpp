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
  graph->getNodeArray().setNodeSizeMethod(getNodeArray().getNodeSizeMethod());
  graph->getNodeArray().setAlpha3(getNodeArray().getAlpha2());

  for (auto it = getNodeArray().getTable().getColumns().begin(); it != getNodeArray().getTable().getColumns().end(); it++) {
    graph->getNodeArray().getTable().addColumn(it->second->create());
  }

  return graph;
}

