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

#if 0
  virtual const std::vector<float> & getWeights() const { return dummy2; }
#endif
  
  // return pointers to the first neighbor and first weight of the node
#if 0
  virtual std::pair<std::vector<unsigned int>::iterator, std::vector<float>::iterator > neighbors(unsigned int node) {
    return std::make_pair<std::vector<unsigned int>::iterator, std::vector<float>::iterator>(dummy1.begin(), dummy2.begin());
  }
#else
  virtual std::vector<std::pair<int, float> > neighbors2(int node) {
    std::vector<std::pair<int, float> > r;
    return r;
  }
#endif  
  
#if 0
  virtual std::vector<unsigned long long> & getDegrees() = 0;
  virtual const std::vector<unsigned long long> & getDegrees() const = 0;
  virtual void addLinks(const std::map<int, float> & m) = 0;
#endif

 public:
#if 0
  std::vector<unsigned int> dummy1;
  std::vector<float> dummy2;
#endif
};

#endif
