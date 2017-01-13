#ifndef _MAPINFOLOADER_H_
#define _MAPINFOLOADER_H_

#include "FileTypeHandler.h"

#include <map>
#include <fstream>
#include <string>

class MapInfoContext {
 public:
  MapInfoContext() { }

 private:
  int version;
  char delimiter;
  std::string charset;
};

class MapInfoLoader : public FileTypeHandler {
 public:
  enum MIFCommand {
    MIF_VERSION = 1,
    MIF_DELIMITER,
    MIF_COLUMNS,
    MIF_COORDSYS,
    MIF_CHARSET,
    MIF_DATA
  };
  enum MIFPrimitive {
    MIF_POINT = 1,
    MIF_LINE,
    MIF_PLINE,
    MIF_REGION,
    MIF_PEN,
    MIF_BRUSH
  };

  MapInfoLoader();
  
  std::shared_ptr<Graph> openGraph(const char * filename, const std::shared_ptr<NodeArray> & initial_nodes) override;

 protected:
  bool handleVersion(std::ifstream & in);
  bool handleDelimiter(std::ifstream & in);
  bool handleCoordSys(std::ifstream & in, Graph & graph);
  bool handleCharset(std::ifstream & in);
  bool handleColumns(std::ifstream & in, Graph & graph);
  bool handleData(std::ifstream & in);

  bool handlePoint(std::ifstream & in, Graph & graph);
  bool handleLine(std::ifstream & in, Graph & graph);
  bool handlePolyline(std::ifstream & in, Graph & graph);
  bool handleRegion(std::ifstream & in, Graph & graph);
  bool handlePen(std::ifstream & in);
  bool handleBrush(std::ifstream & in);
  bool handleCommand(MIFCommand cmd, std::ifstream & in, Graph & graph);
  bool handlePrimitive(MIFPrimitive cmd, std::ifstream & in, Graph & graph);

 private:
  std::map<std::string, MIFCommand> commands;
  std::map<std::string, MIFPrimitive> primitives;
};

#endif
