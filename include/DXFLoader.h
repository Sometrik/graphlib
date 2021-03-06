#ifndef _DXFLOADER_H_
#define _DXFLOADER_H_

#include "FileTypeHandler.h"

#include <string>
#include <list>
#include <fstream>
#include <map>

#include <glm/glm.hpp>

class DXFLayer;
class DXFFace;
class DXFLine;

class DXFLoader : public FileTypeHandler {
 public:
  DXFLoader();
  
  std::shared_ptr<Graph> openGraph(const char * filename, const std::shared_ptr<NodeArray> & initial_nodes) override;
  
 private:
  bool parseHeader(std::ifstream & stream);
  bool parseTables(std::ifstream & stream, std::list<DXFLayer> & layers);
  bool parseEntities(std::ifstream & stream, std::list<DXFLayer> & layers, Graph & graph, std::map<std::string, int> & waiting_faces);
  const DXFLayer & getLayer(const std::string & layer, std::list<DXFLayer> & layers) const;
  void process3DFace(Graph & graph, std::map<std::string, int> & waiting_faces, const DXFFace & face);
  void processLine(Graph & graph, const DXFLine & line);
};

#endif
