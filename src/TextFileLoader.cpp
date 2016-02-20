#include "TextFileLoader.h"

#include <cassert>
#include <iostream>
#include <fstream>
#include <glm/glm.hpp>

#include "DirectedGraph.h"
#include "../system/StringUtils.h"

using namespace std;

TextFileLoader::TextFileLoader() : FileTypeHandler("Text file", false) {
  addExtension("txt");
}

std::shared_ptr<Graph>
TextFileLoader::openGraph(const char * filename, const std::shared_ptr<NodeArray> & initial_nodes) {
  auto graph = std::make_shared<DirectedGraph>();
  graph->setNodeArray(std::make_shared<NodeArray>());

  ifstream in(filename, ios::in);
  if (!in) {
    cerr << "Cannot open " << filename << endl;
    return 0;
  }

  auto & text_data = graph->getFaceData().addTextColumn("text");
  
  while (!in.eof() && !in.fail()) {
    string s;
    getline(in, s);
    StringUtils::trim(s);
    if (s.empty()) continue;

    int face_id = graph->addFace();
    text_data.setValue(face_id, s);

    cerr << "added node " << face_id << ": " << s << endl;
  }
  
  return graph;
}
