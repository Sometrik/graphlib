#include "Rect2d.h"

#include <cmath>
#include <iostream>
#include <cstring>
#include <sstream>

using namespace std;

Rect2d::Rect2d()
  : left(0), // west
    bottom(0), // south
    right(0), // east
    top(0) // north
{
}

Rect2d::Rect2d(const double _left, const double _bottom, const double _right, const double _top)
  : left(_left),
    bottom(_bottom),
    right(_right),
    top(_top)
{
}

Rect2d::Rect2d(const glm::vec3 & point)
  : left(point.x),
    bottom(point.y),
    right(point.x),
    top(point.y)
{

}

bool
Rect2d::intersects(const Rect2d & r) const {
  // fix (might not really be min/max)
  if (right < r.left) return false;
  if (top < r.bottom) return false;
  if (left > r.right) return false;
  if (bottom > r.top) return false;
  return true;
}

bool
Rect2d::within(const Rect2d & r) const {
  if (left >= r.left && right <= r.right && bottom >= r.bottom && top <= r.top) {
    return true;
  } else {
    return false;
  }
}

bool
Rect2d::lineIntersects(double x1, double y1, double x2, double y2) {
  if (x1 < left && x2 < left) return false;
  if (x1 >= right && x2 >= right) return false;
  if (y1 < bottom && y2 < bottom) return false;
  if (y1 >= top && y2 >= top) return false;
  
  return true;
}

glm::dvec3
Rect2d::getCenter() const {
  return glm::dvec3((left + right) / 2, (bottom + top) / 2, 0);
}

void
Rect2d::addTo(const Rect2d & r) {
  // fix
  if (r.left < left) left = r.left;
  if (r.bottom < bottom) bottom = r.bottom;
  if (r.right > right) right = r.right;
  if (r.top > top) top = r.top;
}

double
Rect2d::calcArea() const {
  return fabs(right - left) * fabs(top - bottom);
}

void
Rect2d::scale(double s) {
  // odd scaling??
  left *= s;
  bottom *= s;
  right *= s;
  top *= s;
}

void
Rect2d::growToContain(double x, double y) {
  if (isZero()) {
    left = right = x;
    bottom = top = y;
  } else {
    // inversion problem (heh)
    if (x < left) left = x;
    else if (x > right) right = x;
    if (y < bottom) bottom = y;
    else if (y > top) top = y;
  }
}

string
Rect2d::getWKT() const {
  ostringstream s;
  s.precision(6);
  s << fixed << "Polygon((";
  s << left << " " << bottom << ",";
  s << right << " " << bottom << ",";
  s << right << " " << top << ",";
  s << left << " " << top << ",";
  s << left << " " << bottom;
  s << "))";
  return s.str();
}

void
Rect2d::scaleBy(float f) {
  double x = (right + left) / 2;
  double y = (top + bottom) / 2;
  left = x - (x - left) * f;
  right = x + (right - x) * f;
  bottom = y - (y - bottom) * f;
  top = y + (top - y) * f;
}

void
Rect2d::print() const {
  cerr << fixed << "rect(left = " << left << ", right = " << right << ", bottom = " << bottom << ", top = " << top << ")\n";
}
  
