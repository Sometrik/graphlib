#ifndef _FILEMANAGER_H_
#define _FILEMANAGER_H_

#include <FileTypeHandler.h>

#include <string>
#include <map>
#include <memory>

class Graph;

class FileManager { 
 public:
  static FileManager & getInstance();
  
  std::shared_ptr<Graph> openGraph(const std::string & filename);
  std::shared_ptr<Graph> openGraph(const std::string & filename, const std::shared_ptr<NodeArray> & initial_nodes);

  bool saveGraph(const Graph & graph, const std::string & filename);

  void addHandler(std::shared_ptr<FileTypeHandler> handler);

  FileTypeHandler & getHandlerByFilename(const std::string & filename);
  FileTypeHandler & getHandlerByExtension(const std::string & extension);
  
  std::string getReadableFormatsString() const;
  std::string getWritableFormatsString() const;

 private:
  FileManager();
  ~FileManager();

  static std::string getExtension(const std::string & filename);
  
  static FileManager * instance;
  
  DummyHandler null_handler;

  std::map<std::string, std::shared_ptr<FileTypeHandler> > extmap;
};

#endif
