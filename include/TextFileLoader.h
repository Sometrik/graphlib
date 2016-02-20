#ifndef _TEXTFILELOADER_H_
#define _TEXTFILELOADER_H_

#include "FileTypeHandler.h"

class TextFileLoader : public FileTypeHandler {
 public:
  TextFileLoader();
  
  std::shared_ptr<Graph> openGraph(const char * filename, const std::shared_ptr<NodeArray> & initial_nodes) override;
};

#endif
