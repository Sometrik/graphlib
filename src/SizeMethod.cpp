#include "SizeMethod.h"

#include <Graph.h>

using namespace std;

float
SizeMethod::calculateSize(const node_tertiary_data_s & data, double total_indegree, double total_outdegree, size_t node_count) const {
  switch (method) {
  case SIZE_FROM_DEGREE:
    {
      float degree = data.indegree + data.outdegree;      
      float a;
      if (degree) {
	double avg_degree = float(total_indegree + total_outdegree) / node_count;
	a = degree / avg_degree;
      } else { // if this degree is zero, total degree might also be
	a = 0;
      }
      return 9.0f + 6.0f * log(1.0 + a) / log(3.0);
    }
  case SIZE_FROM_INDEGREE:
    {
      float degree = data.indegree;
      float a;
      if (degree) {
	float avg_degree = float(total_indegree / node_count);
	a = degree / avg_degree;
      } else {
	a = 0;
      }
      return 9.0f + 6.0f * log(1.0 + a) / log(1.4);
    }
  case SIZE_FROM_NODE_COUNT:
    {
#if 0
      auto & subgraph = data.nested_graph;
      float a = subgraph.get() ? subgraph->getNodeCount() : 0;
      return 9.0f + 6.0f * log(1.0 + a) / log(1.4);
#else
      return 9.0f;
#endif
    }
  default:
    return 1.0f;
  }
}
