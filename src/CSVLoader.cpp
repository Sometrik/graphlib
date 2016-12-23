#include "CSVLoader.h"

#include <cassert>
#include <iostream>
#include <fstream>
#include <glm/glm.hpp>

#include "DirectedGraph.h"
#include <StringUtils.h>

using namespace std;

CSVLoader::CSVLoader() : FileTypeHandler("Comma separated values", false) {
  addExtension("csv");
}

std::shared_ptr<Graph>
CSVLoader::openGraph(const char * filename, const std::shared_ptr<NodeArray> & initial_nodes) {
  auto graph = std::make_shared<DirectedGraph>();
  graph->setNodeArray(initial_nodes);

  ifstream in(filename, ios::in);
  if (!in) {
    cerr << "Cannot open " << filename << endl;
    return 0;
  }
  
  char delimiter = '\t';
  vector<string> header;

  cerr << "reading CSV\n";

  try {
    while (!in.eof() && !in.fail()) {
      string s;
      getline(in, s);
      StringUtils::trim(s);
      if (s.empty()) continue;
      vector<string> row = StringUtils::split(s, delimiter);
      assert(!row.empty());
      
      if (header.empty()) {
	header = row;
	for (auto it = header.begin(); it != header.end(); it++) {
	  string n = StringUtils::toLower(*it);
	  if (n == "likes" || n == "count") {
	    graph->getFaceData().addIntColumn(it->c_str());
	  } else if (n != "x" && n != "y" && n != "z" && n != "lat" && n != "lon" &&
		     n != "long" && n != "lng" && n != "latitude" && n != "longitude") {
	    graph->getFaceData().addTextColumn(it->c_str());
	  }
	}
      } else {
	int face_id = graph->addFace();
	double x = 0, y = 0;
	for (unsigned int i = 0; i < row.size(); i++) {
	  if (row[i].empty()) continue;
	  if (header[i] == "X") {
	    x = stof(row[i]);
	  } else if (header[i] == "Y") {
	    y = stof(row[i]);
	  } else {
	    graph->getFaceData()[header[i]].setValue(face_id, row[i]);	  
	  }
	}
	graph->getFaceAttributes(face_id).centroid = glm::vec2(x, y);
      }
    }
  } catch (exception & e) {
    cerr << "exception: " << e.what() << endl;
    assert(0);
  }
  
  cerr << "reading CSV done\n";
  
  return graph;
}
