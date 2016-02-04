#ifndef _UNDIRECTEDGRAPH_H_
#define _UNDIRECTEDGRAPH_H_

#include <Graph.h>

class UndirectedGraph : public Graph {
 public:
  UndirectedGraph(int _id = 0);
  UndirectedGraph(const UndirectedGraph & other);
   
  std::shared_ptr<Graph> createSimilar() const override;
  Graph * copy() const override { return new UndirectedGraph(*this); }
    
  int addUndirectedEdge(int n1, int n2) {
    int hyperedge_id = addFace(-1);
    addEdge(n1, n2, hyperedge_id);
    addEdge(n2, n1, hyperedge_id);
    // TODO: pair the edges
    return hyperedge_id;
  }  
};

#endif
