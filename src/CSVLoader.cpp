#include "CSVLoader.h"

#include <cassert>
#include <iostream>
#include <fstream>
#include <glm/glm.hpp>

#include "PointCloud.h"
#include "../system/StringUtils.h"

using namespace std;
using namespace table;

CSVLoader::CSVLoader() : FileTypeHandler("Comma separated values", false) {
  addExtension("csv");
}

std::shared_ptr<Graph>
CSVLoader::openGraph(const char * filename) {
  auto graph = std::make_shared<PointCloud>();

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
      for (vector<string>::const_iterator it = header.begin(); it != header.end(); it++) {
	string n = StringUtils::toLower(*it);
	if (n == "likes" || n == "count") {
	  graph->getNodeData().addIntColumn(it->c_str());
	} else if (n != "x" && n != "y" && n != "z" && n != "lat" && n != "lon" &&
		   n != "long" && n != "lng" && n != "latitude" && n != "longitude") {
	  graph->getNodeData().addTextColumn(it->c_str());
	}
      }
    } else {
      int node_id = graph->addNode();
      double x = 0, y = 0;
      for (unsigned int i = 0; i < row.size(); i++) {
	if (row[i].empty()) continue;
	if (header[i] == "X") {
	  x = stof(row[i]);
	} else if (header[i] == "Y") {
	  y = stof(row[i]);
	} else {
	  graph->getNodeData()[header[i]].setValue(node_id, row[i]);	  
	}
      }
      graph->setPosition(node_id, glm::vec3(x, y, 0));
    }
  }

  } catch (exception & e) {
    cerr << "exception: " << e.what() << endl;
    assert(0);
  }

  cerr << "reading CSV done\n";
  
  return graph;
}
