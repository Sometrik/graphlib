#ifndef _FILETYPEHANDLER_H_
#define _FILETYPEHANDLER_H_

#include <vector>
#include <string>
#include <map>
#include <memory>

class ArcData2D;
class Graph;
class NodeArray;

class FileTypeHandler {
 public:
  FileTypeHandler(const std::string & _description, bool can_write);
  virtual ~FileTypeHandler() { }
  
  virtual std::shared_ptr<Graph> openGraph(const char * filename, const std::shared_ptr<NodeArray> & initial_nodes) = 0;
  
  std::shared_ptr<Graph> openGraph(const std::string & filename, const std::shared_ptr<NodeArray> & initial_nodes) { return openGraph(filename.c_str(), initial_nodes); }

  std::shared_ptr<Graph> openGraph(const std::string & filename);

  virtual bool saveGraph(const Graph & graph, const std::string & filename) { return false; }
  
  const std::vector<std::string> & getExtensions() const { return extensions; }
  const std::string & getDescription() const { return description; }
  bool canWrite() const { return can_write; }

 protected:
  static int createNode2D(Graph & graph, std::map<std::string, int> & nodes, double x, double y);
  static int createNode3D(Graph & graph, std::map<std::string, int> & nodes, double x, double y, double z = 0);
  static std::pair<int, int> createNodesForArc(const ArcData2D & arc, Graph & graph, std::map<std::string, int> & nodes, bool rev = false);

  void addExtension(const std::string & str) { extensions.push_back(str); }
  
 private:
  std::string description;
  std::vector<std::string> extensions;
  bool can_write;
};

#if 0
class FileTypeHandlerFactory {
 public:
 FileTypeHandlerFactory(bool _can_read, bool _can_write) : can_read(_can_read), can_write(_can_write) { }

  bool canRead() const { return can_read; }
  bool canWrite() const { return can_write; }

 protected:
  void canRead(bool t) { can_read = t; }
  void canWrite(bool t) { can_write = t; }

 private:
  bool can_read, can_write;
};
#endif

class DummyHandler : public FileTypeHandler {
 public:
 DummyHandler() : FileTypeHandler("", false) { }
  
  std::shared_ptr<Graph> openGraph(const char * filename, const std::shared_ptr<NodeArray> & initial_nodes) override { return std::shared_ptr<Graph>(0); }
};

#endif
