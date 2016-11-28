#ifndef _DIRECTEDGRAPH_H_
#define _DIRECTEDGRAPH_H_

#include <Graph.h>
#include <GraphFilter.h>

class DirectedGraph : public Graph {
 public:
  DirectedGraph(int _id = 0);
  DirectedGraph(const DirectedGraph & other);

  std::shared_ptr<Graph> createSimilar() const override;
  Graph * copy() const override { return new DirectedGraph(*this); }
  bool isDirected() const override { return true; }
  
 private:
};

#endif
