#ifndef _RECT2D_H_
#define _RECT2D_H_

#include <string>
#include <glm/glm.hpp>

class Rect2d {
 public:
  Rect2d();
  Rect2d(double _left, double _bottom, double _right, double _top);
  Rect2d(const glm::vec3 & point);
  
  void clear() { left = right = bottom = top = 0; }

  bool contains(double x, double y) const { return x >= left && y >= bottom && x <= right && y <= top; }
  bool intersects(const Rect2d & r) const;
  bool within(const Rect2d & r) const;
  bool lineIntersects(double x1, double y1, double x2, double y2);

  glm::dvec3 getCenter() const;
  void addTo(const Rect2d & r);
  void scale(double s);
  
  bool isZero() const { return left == 0 && bottom == 0 && right == 0 && top == 0; }
  
  double calcArea() const;

  void growToContain(double x, double y);
  void growToContain(const glm::vec3 & v) { growToContain(v.x, v.y); }

  double getWidth() const { return right > left ? right - left : left - right; }
  double getHeight() const  { return top > bottom ? top - bottom : bottom - top; }

  std::string getWKT() const;

  void scaleBy(float f);

  void print() const;
  
  double left, bottom, right, top;
};

#endif
