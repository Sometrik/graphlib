#include "TextFileLoader.h"

#include <cassert>
#include <iostream>
#include <fstream>
#include <glm/glm.hpp>

#include "PointCloud.h"
#include "../system/StringUtils.h"

using namespace std;

TextFileLoader::TextFileLoader() : FileTypeHandler("Text file", false) {
  addExtension("txt");
}

std::shared_ptr<Graph>
TextFileLoader::openGraph(const char * filename) {
  auto graph = std::make_shared<PointCloud>();
  graph->setNodeArray(std::make_shared<NodeArray>());

  ifstream in(filename, ios::in);
  if (!in) {
    cerr << "Cannot open " << filename << endl;
    return 0;
  }

  auto & text_data = graph->getNodeArray().getTable().addTextColumn("text");
  
  while (!in.eof() && !in.fail()) {
    string s;
    getline(in, s);
    StringUtils::trim(s);
    if (s.empty()) continue;

    int node_id = graph->addNode();
    text_data.setValue(node_id, s);

    cerr << "added node " << node_id << ": " << s << endl;
  }
  
  return graph;
}
