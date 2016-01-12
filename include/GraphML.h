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
  std::shared_ptr<Graph> openGraph(const char * filename) override;
  bool saveGraph(const Graph & graph, const std::string & filename) override;

 protected:
  std::shared_ptr<Graph> createGraphFromElement(tinyxml2::XMLElement & graphml_element, tinyxml2::XMLElement & graph_element) const;

 private:
};

#endif
