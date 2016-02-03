#ifndef _DIRECTEDGRAPH_H_
#define _DIRECTEDGRAPH_H_

#include <Graph.h>

#include <unordered_set>
#include <unordered_map>

class DirectedGraph : public Graph {
 public:
  DirectedGraph(int _id = 0);
  DirectedGraph(const DirectedGraph & other);

  std::shared_ptr<Graph> createSimilar() const override;
  Graph * copy() const override { return new DirectedGraph(*this); }
  bool isDirected() const override { return true; }
  bool hasPosition() const override { return current_pos != -1; }

  bool updateData(time_t start_time, time_t end_time, float start_sentiment, float end_sentiment, Graph & source_graph, RawStatistics & stats, bool is_first_level, Graph * base_graph) override;

  int addEdge(int n1, int n2, int face = -1, float weight = 1.0f, int arc = 0) override {
    assert(n1 != -1 && n2 != -1);
    int edge = (int)edge_attributes.size();
    int next_node_edge = getNodeFirstEdge(n1);
    int next_face_edge = -1;
    if (face != -1) next_face_edge = getFaceFirstEdge(face);
    setNodeFirstEdge(n1, edge);
    if (n1 != n2) {
      updateOutdegree(n1, 1.0f); // weight);
      updateIndegree(n2, 1.0f); // weight);
    }
    updateNodeSize(n1);
    updateNodeSize(n2);	
    
    edge_attributes.push_back(edge_data_s( weight, n1, n2, next_node_edge, face, next_face_edge, arc ));
    edges.addRow();
    total_edge_weight += fabsf(weight);

    if (face != -1) {
      face_attributes[face].first_edge = edge;
    }

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

  void reset() override {
    seen_nodes.clear();
    seen_edges.clear();
    current_pos = -1;
    num_links = 0;
    num_hashtags = 0;
    min_time = 0;
    max_time = 0;
  }
  
 private:
  std::vector<edge_data_s> edge_attributes;
  std::unordered_set<int> seen_nodes;
  std::unordered_map<int, std::unordered_map<int, int> > seen_edges;
  int current_pos = -1;
  unsigned int num_links = 0, num_hashtags = 0;
  time_t min_time = 0, max_time = 0;
};

#endif
