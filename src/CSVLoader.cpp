#include "CSVLoader.h"

#include <cassert>
#include <iostream>
#include <fstream>
#include <glm/glm.hpp>

#include <Graph.h>
#include <StringUtils.h>

using namespace std;

CSVLoader::CSVLoader() : FileTypeHandler("Comma separated values", false) {
  addExtension("csv");
}

std::shared_ptr<Graph>
CSVLoader::openGraph(const char * filename, const std::shared_ptr<NodeArray> & initial_nodes) {
  auto graph = std::make_shared<Graph>();
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

#if 0
      auto & table = graph->getFaceData();
#else
      auto & table = graph->getNodeArray().getTable();
#endif

      if (header.empty()) {
	header = row;
	for (auto it = header.begin(); it != header.end(); it++) {
	  string n = StringUtils::toLower(*it);
	  cerr << "adding column " << n << endl;
	  if (n == "likes" || n == "count") {
	    table.addIntColumn(it->c_str());
	  } else if (n != "x" && n != "y" && n != "z" && n != "lat" && n != "lon" &&
		     n != "long" && n != "lng" && n != "latitude" && n != "longitude") {
	    table.addTextColumn(it->c_str());
	  }
	}
      } else {
#if 0
	int row_id = graph->addFace();
#else
	int row_id = graph->getNodeArray().add();
#endif
	double x = 0, y = 0;
	for (unsigned int i = 0; i < row.size(); i++) {
	  if (row[i].empty()) continue;
	  if (header[i] == "X") {
	    x = stof(row[i]);
	  } else if (header[i] == "Y") {
	    y = stof(row[i]);
	  } else {
	    table[header[i]].setValue(row_id, row[i]);	  
	  }
	}
#if 0
	graph->getFaceAttributes(row_id).centroid = glm::vec2(x, y);
#endif
      }
    }
  } catch (exception & e) {
    cerr << "exception: " << e.what() << endl;
    assert(0);
  }
  
  cerr << "reading CSV done\n";
  
  return graph;
}
