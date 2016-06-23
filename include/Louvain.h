#ifndef _LOUVAIN_H_
#define _LOUVAIN_H_

#include <unordered_map>
#include <vector>

class Graph;

class Louvain {
 public:
  Louvain(Graph * _g, int number_of_passes, double min_modularity);
 
  // return the neighboring communities of a node
  // and the number of links from the node to each community
  std::unordered_map<int, float> neighboringCommunities(int node) const;
  
  // compute the communities for each node that has no parent
  bool oneLevel();

  Graph & getGraph() { return *g; }
  const Graph & getGraph() const { return *g; }

 protected:
  // return the community of given node
  int getNodeCommunity(int node) const;

 private:
  Graph * g;
  std::vector<int> nodes;
  
  // number of passes or -1 to do as many passes as needed to increase modularity
  int number_of_passes;

  // minimum improvement for doing another pass
  double min_modularity;
};

#endif
