#ifndef _MULTIARCGRAPH_H_
#define _MULTIARCGRAPH_H_

#include <Graph.h>

class MultiArcGraph : public Graph {
 public:
  MultiArcGraph(int _id = 0);
  MultiArcGraph(const MultiArcGraph & other);
   
  std::shared_ptr<Graph> createSimilar() const override;
  Graph * copy() const override { return new MultiArcGraph(*this); }
 
  int addEdge(int n1, int n2, int face = -1, float weight = 1.0f, int arc = 0, long long coverage = 0) override;
  
  void clear() override {
    Graph::clear();
    edge_attributes.clear();
  }  
  
  edge_data_s & getEdgeAttributes(int i) override { return edge_attributes[i]; }
  const edge_data_s & getEdgeAttributes(int i) const override { return edge_attributes[i]; }
  EdgeIterator begin_edges() override { return EdgeIterator(&(edge_attributes.front()), sizeof(edge_data_s)); }
  EdgeIterator end_edges() override { 
    EdgeIterator it(&(edge_attributes.back()), sizeof(edge_data_s));
    ++it;
    return it;
  }
  ConstEdgeIterator begin_edges() const override { return ConstEdgeIterator(&(edge_attributes.front()), sizeof(edge_data_s)); }
  ConstEdgeIterator end_edges() const override { 
    ConstEdgeIterator it(&(edge_attributes.back()), sizeof(edge_data_s));
    ++it;
    return it;
  }

 private:
  std::vector<edge_data_s> edge_attributes;
};

#endif
