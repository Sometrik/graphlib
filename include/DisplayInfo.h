#ifndef _DISPLAYINFO_H_
#define _DISPLAYINFO_H_

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class DisplayInfo {
 public:
  enum ViewMode {
    UNKNOWN_VIEW = 0,
    TOPOLOGICAL_VIEW,
    GEOGRAPHICAL_VIEW
  };

 DisplayInfo(ViewMode _mode = UNKNOWN_VIEW) : mode(_mode) { }

  glm::vec3 unProject(double x, double y, double z = 0) const {
    return glm::unProject(glm::vec3(x, y, z), mvmatrix, projmatrix, viewport);
  }

  glm::vec3 project(const glm::vec3 & wp) const {
    glm::vec3 v = glm::project(wp, mvmatrix, projmatrix, viewport);
    v *= 1.0f / display_scale;
    return v;
  }

  glm::vec3 project2(const glm::vec3 & wp) const {
    return glm::project(wp, mvmatrix, projmatrix, viewport);
  }

  bool isPointVisible(const glm::vec3 & p) const {
    glm::vec3 p2 = project2(p);
    return p2.x >= viewport[0] && p2.y >= viewport[1] && p2.x <= viewport[0] + viewport[2] && p2.y <= viewport[1] + viewport[3];
  }

  bool isPointVisible(const glm::vec2 & p) const { return isPointVisible(glm::vec3(p, 0.0f)); }
  
  void setViewport(const glm::ivec4 & v) { viewport = v; }
  const glm::ivec4 & getViewport() const { return viewport; }

  void setModelViewMatrix(const glm::mat4 & m) { mvmatrix = m; }
  void setProjectionMatrix(const glm::mat4 & m) { projmatrix = m; }
  void setDisplayScale(float f) { display_scale = f; }

  const glm::mat4 & getProjectionMatrix() const { return projmatrix; }

  const ViewMode getViewMode() const { return mode; }
  void setViewMode(ViewMode _mode) { mode = _mode; }
  
 private:
  ViewMode mode;
  glm::ivec4 viewport;
  glm::mat4 mvmatrix, projmatrix;
  float display_scale = 1.0f;
};

#endif
