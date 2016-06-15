#ifndef _GRAPHINTERFACE_H_
#define _GRAPHINTERFACE_H_

#include "MBRObject.h"

#include <vector>
#include <map>

class GraphInterface : public MBRObject {
 public:
  virtual ~GraphInterface() { }
  
  virtual size_t getNodeCount() const = 0;
  virtual double getTotalWeight() const = 0;

  // return the number of self loops of the node
  virtual float nb_selfloops(int node) = 0;

  // return the weighted degree of the node
  virtual float weighted_degree(int node) = 0;
  
  // return the number of neighbors (degree) of the node
  virtual unsigned int nb_neighbors(int node) = 0;
  
  // return indices to neighbours as well as weights of the corresponding edges
  virtual std::vector<std::pair<int, float> > neighbors2(int node) = 0;  
};

#endif
