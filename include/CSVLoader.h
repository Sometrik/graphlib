#ifndef _CSVLOADER_H_
#define _CSVLOADER_H_

#include "FileTypeHandler.h"

class CSVLoader : public FileTypeHandler {
 public:
  CSVLoader();
  std::shared_ptr<Graph> openGraph(const char * filename) override;
};

#endif
