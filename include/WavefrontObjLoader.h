#ifndef _WAVEFRONTOBJLOADER_H_
#define _WAVEFRONTOBJLOADER_H_

#include "FileTypeHandler.h"

class WavefrontObjLoader : public FileTypeHandler {
 public:
  WavefrontObjLoader();
  
  std::shared_ptr<Graph> openGraph(const char * filename) override;
};

#endif
