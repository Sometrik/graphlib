#ifndef _DIRECTEDGRAPH_H_
#define _DIRECTEDGRAPH_H_

#include <Graph.h>

class DirectedGraph : public Graph {
 public:
  DirectedGraph(int _id = 0) : Graph(_id) { }

  std::shared_ptr<Graph> createSimilar() const override;
  bool isDirected() const override { return true; }
  
 private:
};

#endif
