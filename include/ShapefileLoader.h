#ifndef _SHAPEFILELOADER_H_
#define _SHAPEFILELOADER_H_

#include "FileTypeHandler.h"

class ShapefileLoader : public FileTypeHandler {
 public:
  ShapefileLoader();
  
  std::shared_ptr<Graph> openGraph(const char * filename) override;

 private:
};

#endif
