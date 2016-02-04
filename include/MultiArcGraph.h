#ifndef _MULTIARCGRAPH_H_
#define _MULTIARCGRAPH_H_

#include <Graph.h>

class MultiArcGraph : public Graph {
 public:
  MultiArcGraph(int _id = 0);
  MultiArcGraph(const MultiArcGraph & other);
   
  std::shared_ptr<Graph> createSimilar() const override;
  Graph * copy() const override { return new MultiArcGraph(*this); }
};

#endif
