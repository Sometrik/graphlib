#ifndef _ARCDATA_H_
#define _ARCDATA_H_

#include <vector>
#include <glm/glm.hpp>

class ArcData {
 public:
  ArcData() { }

  size_t size() const { return data.size(); }
  bool empty() const { return data.empty(); }
  
  std::vector<glm::dvec2> data;
};

#endif
