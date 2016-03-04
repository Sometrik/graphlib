#ifndef _NODETYPE_H_
#define _NODETYPE_H_

enum NodeType : unsigned short {
  NODE_ANY = 0,
  NODE_ANY_MALE,
  NODE_ANY_FEMALE,
  NODE_ANY_PAGE,
  NODE_URL,
  NODE_HASHTAG,
  NODE_TOKEN,
  NODE_USER
};

#endif