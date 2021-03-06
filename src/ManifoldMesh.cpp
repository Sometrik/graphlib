#include "ManifoldMesh.h"

using namespace std;

ManifoldMesh::ManifoldMesh(int _id) : Graph(3, _id) {
#if 0
  edges.addIntColumn("_nextEdgeA");
  edges.addIntColumn("_nextEdgeB");
  edges.addIntColumn("_nodeA");
  edges.addIntColumn("_nodeB");
  edges.addIntColumn("_leftFace");
  edges.addIntColumn("_rightFace");
  edges.addIntColumn("_nextLeftFaceEdge");
  edges.addIntColumn("_nextRightFaceEdge");  
  faces.addIntColumn("_frontShell");
  faces.addIntColumn("_backShell");
  // shells.addIntColumn("_firstFace");
#endif
}

std::shared_ptr<Graph>
ManifoldMesh::createSimilar() const {
  std::shared_ptr<Graph> graph(new ManifoldMesh(getId()));
  graph->setLocationGraphValid(false);
  graph->setTemporal(isTemporal());
  graph->setPersonality(getPersonality());
  graph->setHasTextures(hasTextures());
  graph->setCustomNodeSizeColumn(getCustomNodeSizeColumn());
  graph->setNodeSizeMethod(getNodeSizeMethod());
  graph->setClusterVisibility(getClusterVisibility());
  graph->setNodeVisibility(getNodeVisibility());
  graph->setEdgeVisibility(getEdgeVisibility());
  graph->setRegionVisibility(getRegionVisibility());
  graph->setLabelVisibility(getLabelVisibility());
  graph->setNodeArray(nodes);
  
  return graph;
}

bool
ManifoldMesh::checkConsistency() const {
  // V – E + F = H + 2 * (S – G)
  return true;
}
