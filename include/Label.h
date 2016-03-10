#ifndef _LABEL_H_
#define _LABEL_H_

#define LABEL_FLAG_CENTER	1
#define LABEL_FLAG_MIDDLE	2

class Label {
 public:
  glm::vec3 pos;
  glm::vec2 offset;
  int texture;
  unsigned short flags;
  glm::vec4 color1, color2;
};

#endif
