#include "FileManager.h"

#include <Graph.h>

using namespace std;

FileManager * FileManager::instance = 0;

FileManager::FileManager() {

}

FileManager &
FileManager::getInstance() {
  if (!instance) instance = new FileManager;
  return *instance;
}

std::shared_ptr<Graph>
FileManager::openGraph(const string & filename) {
  auto initial_nodes = std::make_shared<NodeArray>();
  return openGraph(filename, initial_nodes);
}

std::shared_ptr<Graph>
FileManager::openGraph(const string & filename, const std::shared_ptr<NodeArray> & initial_nodes) {
  return getHandlerByFilename(filename).openGraph(filename, initial_nodes);
}

bool
FileManager::saveGraph(const Graph & graph, const string & filename) {
  return getHandlerByFilename(filename).saveGraph(graph, filename);  
}

string
FileManager::getExtension(const string & filename) {
  string::size_type pos = filename.find_last_of('.');
  if (pos == string::npos) {
    return "";    
  } else {
    return filename.substr(pos + 1);
  }
}

FileTypeHandler &
FileManager::getHandlerByFilename(const string & filename) {
  string extension = getExtension(filename);
  return getHandlerByExtension(extension);
}

FileTypeHandler &
FileManager::getHandlerByExtension(const string & extension) {
  map<string, shared_ptr<FileTypeHandler> >::iterator it = extmap.find(extension);
  if (it != extmap.end()) {
    return *(it->second);
  } else {
    fprintf(stderr, "no handler for type \"%s\"\n", extension.c_str());
    return null_handler;
  }
}

void
FileManager::addHandler(std::shared_ptr<FileTypeHandler> handler) {
  assert(handler.get());
  const vector<string> & extensions = handler->getExtensions();
  for (auto & ext : extensions) {
    extmap[ext] = handler;
  }
}

string
FileManager::getReadableFormatsString() const {
  string t;
  for (auto & ed : extmap) {
    if (!t.empty()) t += "|";
    t += ed.second->getDescription() + " (*." + ed.first + ")|*." + ed.first;
  }
  if (!t.empty()) t += "|";
  t += "All files (*.*)|*.*";
  return t;
}

string
FileManager::getWritableFormatsString() const {
  string t;
  for (map<string, std::shared_ptr<FileTypeHandler> >::const_iterator it = extmap.begin(); it != extmap.end(); it++) {
    if (it->second->canWrite()) {
      if (!t.empty()) t += "|";
      t += it->second->getDescription() + " (*." + it->first + ")|*." + it->first;
    }
  }
  if (!t.empty()) t += "|";
  t += "All files (*.*)|*.*";
  return t;
}
