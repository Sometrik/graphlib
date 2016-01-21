#ifndef _NODEARRAY_H_
#define _NODEARRAY_H_

#include "NodeType.h"
#include "SizeMethod.h"
#include "../system/Mutex.h"
#include "skey.h"

#include <glm/glm.hpp>
#include <string>

class Graph;

struct node_data_s {
  graph_color_s color; // 0
  glm::uint32 normal; // 4
  glm::vec3 position; // 8
  glm::vec3 prev_position; // 20
  float age, size; // 32
  short texture, flags; // 40
};

struct node_secondary_data_s {
  NodeType type;
  int first_edge, orig_node_id;
  int cluster_id;
  short label_texture;
  unsigned short label_visibility_val;
  float indegree, outdegree;
  std::string label;
  std::shared_ptr<Graph> nested_graph;
};

class NodeArray {
 public:
  friend class Graph;
  NodeArray() { }

 protected:
  void updateNodeSize(int n) { node_geometry[n].size = node_size_method.calculateSize(node_geometry2[n], total_indegree, total_outdegree, getNodeCount() ); }

 private:
  void lockReader() const {
    MutexLocker locker(mutex);
    num_readers++;
    if (num_readers == 1) writer_mutex.lock();
  }
  void unlockReader() const {
    MutexLocker locker(mutex);
    num_readers--;
    if (num_readers == 0) {
      writer_mutex.unlock();
    }
  }
  void lockWriter() {
    writer_mutex.lock();
  }
  void unlockWriter() {
    writer_mutex.unlock();
  }

  int id;
  std::map<skey, int> node_cache;
  table::Table nodes;
  std::vector<node_data_s> node_geometry;
  std::vector<node_secondary_data_s> node_geometry2;
  SizeMethod node_size_method;
  double total_outdegree = 0, total_indegree = 0;

  mutable int num_readers = 0;
  mutable Mutex mutex, writer_mutex;
};

#endif
