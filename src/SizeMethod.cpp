#include "SizeMethod.h"

#include <Graph.h>

using namespace std;

float
SizeMethod::calculateSize(const node_tertiary_data_s & data, unsigned int total_indegree, unsigned int total_outdegree, size_t node_count) const {
  switch (method) {
  case CONSTANT: return constant;    
  case SIZE_FROM_DEGREE:
    {
      unsigned int degree = data.indegree + data.outdegree;      
      float a;
      if (degree) {
	double avg_degree = double(total_indegree + total_outdegree) / node_count;
	a = degree / avg_degree;
      } else { // if this degree is zero, total degree might also be
	a = 0;
      }
      
      return 5.0f + 5.0f * (log(1.0 + a) / log(2.0)) + 5.0f * sqrtf(25 * data.child_count);
    }
  case SIZE_FROM_INDEGREE:
    {
      unsigned int degree = data.indegree;
      float a;
      if (degree) {
	double avg_degree = double(total_indegree) / node_count;
	a = degree / avg_degree;
      } else {
	a = 0;
      }
      return 4.0f + log(1.0 + a) / log(1.3) + 5.0f * sqrtf(25 * data.child_count);
    }
  case SIZE_FROM_NODE_COUNT:
    {
#if 0
      auto & subgraph = data.nested_graph;
      float a = subgraph.get() ? subgraph->getNodeCount() : 0;
      return 5.0f + 5.0f * log(1.0 + a) / log(1.4);
#else
      return 5.0f;
#endif
    }
  default:
    return 1.0f;
  }
}
