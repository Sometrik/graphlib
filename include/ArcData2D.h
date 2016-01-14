#ifndef _ARCDATA2D_H_
#define _ARCDATA2D_H_

#include <vector>
#include <glm/glm.hpp>

class ArcData2D {
 public:
  ArcData2D() { }

  size_t size() const { return data.size(); }
  bool empty() const { return data.empty(); }
  
  std::vector<glm::dvec2> data;
};

#endif
