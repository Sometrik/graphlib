#ifndef _MULTIARCGRAPH_H_
#define _MULTIARCGRAPH_H_

#include <Graph.h>

class MultiArcGraph : public Graph {
 public:
  MultiArcGraph(int _id = 0);
  MultiArcGraph(const MultiArcGraph & other);
   
  std::shared_ptr<Graph> createSimilar() const override;
  Graph * copy() const override { return new MultiArcGraph(*this); }
 
  int addEdge(int n1, int n2, int face = -1, float weight = 1.0f, int arc = 0) override {
    assert(n1 != -1 || n2 != -1);
    // assert(n1 != n2);
    int edge = (int)edge_attributes.size();
    
    int next_node_edge = getNodeFirstEdge(n1);
    setNodeFirstEdge(n1, edge);
    if (n1 != n2) {
      updateOutdegree(n1, 1.0f); // weight
      updateIndegree(n2, 1.0f); // weight
    }
    updateNodeSize(n1);
    updateNodeSize(n2);

    if (face != -1) {
      
    }

    edge_attributes.push_back(edge_data_s( weight, n1, n2, next_node_edge, face, -1, arc ));
    edges.addRow();
    total_edge_weight += fabsf(weight);

    incVersion();
    return edge;
  }
  
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
