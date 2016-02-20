#ifndef _GRAPHML_H_
#define _GRAPHML_H_

#include "FileTypeHandler.h"

class Graph;

namespace tinyxml2 {
  class XMLElement;
};

class GraphML : public FileTypeHandler {
 public:
  GraphML();
  std::shared_ptr<Graph> openGraph(const char * filename, const std::shared_ptr<NodeArray> & initial_nodes) override;
  bool saveGraph(const Graph & graph, const std::string & filename) override;

 protected:
  void createGraphFromElement(Graph & graph, tinyxml2::XMLElement & graphml_element, tinyxml2::XMLElement & graph_element, std::map<std::string, int> & nodes_by_id, bool is_directed, int parent_node_id = -1) const;  
};

#endif
