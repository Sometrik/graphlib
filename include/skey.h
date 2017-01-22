#ifndef _SKEY_H_
#define _SKEY_H_

#include <functional>

class skey {
 public:
 skey() : source_id(0), source_object_id(0) { }
  skey(short _source_id, long long _source_object_id) : source_id(_source_id), source_object_id(_source_object_id) { }
  
  bool operator< (const skey & other) const {
    if (source_id < other.source_id) return true;
    else if (source_id == other.source_id && source_object_id < other.source_object_id) return true;
    return false;
  }
  
  bool operator== (const skey & other) const {
    if (source_id != other.source_id) return false;
    if (source_object_id != other.source_object_id) return false;
    return true;
  }
  bool operator!= (const skey & other) const {
    if (source_id != other.source_id) return true;
    if (source_object_id != other.source_object_id) return true;
    return false;
  }

  void clear() {
    source_id = 0;
    source_object_id = 0;
  }

  bool defined() const { return source_id != 0 && source_object_id != 0; }
  
  short source_id;
  long long source_object_id;
};

namespace std {
  template <>
  struct hash<skey> {
    std::size_t operator()(const skey & a) const {
      return hash<short>()(a.source_id) ^ hash<long long>()(a.source_object_id);
    }
  };
};

#endif
