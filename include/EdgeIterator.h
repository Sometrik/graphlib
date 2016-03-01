#ifndef _EDGEITERATOR_H_
#define _EDGEITERATOR_H_

class EdgeIterator {
 public:
 EdgeIterator() : ptr(0) { }
  EdgeIterator(edge_data_s * _ptr) : ptr(_ptr) { }

  bool isValid() const { return ptr != 0; }

  edge_data_s & operator*() { return *ptr; }
  edge_data_s * operator->() { return ptr; }
  edge_data_s * get() { return ptr; }

  EdgeIterator & operator++() { ptr++; return *this; }
  EdgeIterator & operator--() { ptr--; return *this; }
  
  bool operator==(const EdgeIterator & other) const { return this->ptr == other.ptr; }
  bool operator!=(const EdgeIterator & other) const { return this->ptr != other.ptr; }

  void clear() { ptr = 0; }

 private:
  edge_data_s *  ptr;
};

class ConstEdgeIterator {
 public:
 ConstEdgeIterator() : ptr(0) { }
 ConstEdgeIterator( const edge_data_s * _ptr ) : ptr(_ptr) { }

  bool isValid() const { return ptr != 0; }

  const edge_data_s & operator*() const { return *ptr; }
  const edge_data_s * operator->() const { return ptr; }
  const edge_data_s * get() const { return ptr; }

  ConstEdgeIterator & operator++() { ptr++; return *this; }
  ConstEdgeIterator & operator--() { ptr++; return *this; }
  
  bool operator==(const ConstEdgeIterator & other) const { return this->ptr == other.ptr; }
  bool operator!=(const ConstEdgeIterator & other) const { return this->ptr != other.ptr; }

  void clear() { ptr = 0; }

 private:
  const edge_data_s * ptr;
};

#endif
