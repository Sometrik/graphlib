#ifndef _MBROBJECT_H_
#define _MBROBJECT_H_

#include <Rect2d.h>

class MBRObject {
 public:
  MBRObject() { }
 MBRObject(Rect2d _mbr) : mbr(_mbr) { }
  // virtual ~MBRObject() { } // must be polymorphic
  
  const Rect2d & getMBR() const { return mbr; }
  void setMBR(double min_x, double min_y, double max_x, double max_y) { mbr = Rect2d(min_x, min_y, max_x, max_y); } 
  void setMBR(const Rect2d & r) { mbr = r; }
  
  bool intersectsMBR(const Rect2d & r) const { return mbr.intersects(r); }
  bool intersectsMBR(const MBRObject & o) const { return mbr.intersects(o.mbr); }
  bool withinMBR(const Rect2d & r) const { return mbr.within(r); }

  glm::dvec3 getCenterMBR() const { return mbr.getCenter(); }
  
 protected:
  Rect2d mbr;
};

#endif
