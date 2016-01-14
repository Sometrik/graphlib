#ifndef _TOPOJSONLOADER_H_
#define _TOPOJSONLOADER_H_

#include "FileTypeHandler.h"

#include <json/json.h>
#include <map>
#include <vector>

class TopoJSONLoader : public FileTypeHandler {
 public:
  TopoJSONLoader();
  
  std::shared_ptr<Graph> openGraph(const char * filename) override;

 private:
  void handleCollection(const std::string & parent_id, Graph & graph, std::map<std::string, int> & nodes, std::map<int, std::vector<int> > & connections, Json::Value & objects, const std::vector<ArcData2D> & arc_data);
};

#endif
