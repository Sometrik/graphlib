#ifndef _SIZEMETHOD_H_
#define _SIZEMETHOD_H_

#include <string>

class Graph;
struct node_tertiary_data_s;

class SizeMethod {
 public:
  enum Method {
    CONSTANT = 0,
    SIZE_FROM_DEGREE,
    SIZE_FROM_INDEGREE,
    SIZE_FROM_COLUMN,
    SIZE_FROM_NODE_COUNT
  };
 SizeMethod(Method _method, float _constant = 0.0f) : method(_method), constant(_constant) { }
 SizeMethod(Method _method, const std::string & _column) : method(_method), column(_column), constant(0.0f) { }

  Method getValue() const { return method; }
  const std::string & getColumn() const { return column; }
  float getConstant() const { return constant; }
  bool definedForSource() const { return method == SIZE_FROM_DEGREE; }
  bool definedForTarget() const { return method == SIZE_FROM_DEGREE || method == SIZE_FROM_INDEGREE; }
  
  float calculateSize(const node_tertiary_data_s & data, double total_indegree, double total_outdegree, size_t node_count) const;
  
 private:
  Method method;
  std::string column;
  float constant;
};

#endif
