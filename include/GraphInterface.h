#ifndef _GRAPHINTERFACE_H_
#define _GRAPHINTERFACE_H_

#include "MBRObject.h"

#include <vector>
#include <map>

class GraphInterface : public MBRObject {
 public:
  virtual ~GraphInterface() { }
    
  // return the number of self loops of the node
  virtual float numberOfSelfLoops(int node) = 0;

  // return the weighted degree of the node
  virtual float weightedDegree(int node) = 0;
  
  // return the number of neighbors (degree) of the node
  virtual unsigned int numberOfNeighbors(int node) = 0;
  
  // return indices to neighbours as well as weights of the corresponding edges
  virtual std::vector<std::pair<int, float> > getAllNeighbors(int node) const = 0;  
};

#endif
