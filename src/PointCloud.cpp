#include "PointCloud.h"

using namespace std;
using namespace table;

PointCloud::PointCloud(int _id) : Graph(0, _id) {  

}

PointCloud::PointCloud(const PointCloud & other)
  : Graph(other) {

}

std::shared_ptr<Graph>
PointCloud::createSimilar() const {
  std::shared_ptr<Graph> graph(new PointCloud(getId()));
  graph->setLocationGraphValid(false);
  graph->setAlpha3(getAlpha2());
  graph->setTemporal(isTemporal());
  graph->setPersonality(getPersonality());
  graph->setHasTextures(hasTextures());
  graph->setNodeSizeMethod(getNodeSizeMethod());
  graph->setClusterVisibility(getClusterVisibility());
  graph->setNodeVisibility(getNodeVisibility());
  graph->setEdgeVisibility(getEdgeVisibility());
  graph->setRegionVisibility(getRegionVisibility());
  graph->setLabelVisibility(getLabelVisibility());

  for (auto it = getNodeData().getColumns().begin(); it != getNodeData().getColumns().end(); it++) {
    if (it->first != "posts") {
      graph->getNodeData().addColumn(it->second->create());
    }
  }

  return graph;
}
