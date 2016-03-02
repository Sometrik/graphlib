#ifndef _VISIBLENODEITERATOR_H_
#define _VISIBLENODEITERATOR_H_

class ConstVisibleNodeIterator {
 public:
  ConstVisibleNodeIterator(const edge_data_s * _edge_ptr,
			   const edge_data_s * _edge_end,
			   const node_tertiary_data_s * _node_ptr,
			   const node_tertiary_data_s * _node_end,
			   size_t _num_nodes)
    : stage(_edge_ptr < _edge_end ? EDGE_TAIL : END),
    edge_ptr(_edge_ptr),
    edge_end(_edge_end),
    node_ptr(_node_ptr),
    node_end(_node_end),
    num_nodes(_num_nodes)
  {
    processed_nodes.resize(num_nodes);
    fetchNextNode();
  }
  ConstVisibleNodeIterator()
    : stage(END),
    current_node(-1),
    edge_ptr(0),
    edge_end(0),
    node_ptr(0),
    node_end(0),
    num_nodes(0) { }

  const int & operator*() const { return current_node; }
  const int * get() const { return &current_node; }

  ConstVisibleNodeIterator & operator++() {
    fetchNextNode();
    return *this;
  }

  bool operator==(const ConstVisibleNodeIterator & other) const { return current_node == other.current_node; }
  bool operator!=(const ConstVisibleNodeIterator & other) const { return current_node != other.current_node; }
  
 private:
  void fetchNextNode() {
    current_node = -1;
    while (current_node == -1 && stage != END) {
      if (stage == EDGE_TAIL) {
	int n = edge_ptr->tail;
	if (!processed_nodes[n]) {
	  processed_nodes[n] = true;
	  if (node_ptr + n < node_end && node_ptr[n].parent_node != -1) parent_nodes.push_back(node_ptr[n].parent_node);
	  current_node = n;
	}
	stage = EDGE_HEAD;
      } else if (stage == EDGE_HEAD) {
	int n = edge_ptr->head;
	if (!processed_nodes[n]) {
	  processed_nodes[n] = true;
	  if (node_ptr + n < node_end && node_ptr[n].parent_node != -1) parent_nodes.push_back(node_ptr[n].parent_node);
	  current_node = n;
	}
	edge_ptr++;
	if (edge_ptr != edge_end) {
	  stage = EDGE_TAIL;
	} else {
	  stage = PARENT_NODES;
	}
      } else if (stage == PARENT_NODES) {
	if (parent_nodes.empty()) {
	  stage = END;
	} else {
	  int n = parent_nodes.back();
	  parent_nodes.pop_back();
	  if (!processed_nodes[n]) {
	    processed_nodes[n] = true;
	    if (node_ptr + n < node_end && node_ptr[n].parent_node != -1) parent_nodes.push_back(node_ptr[n].parent_node);
	    current_node = n;
	  }
	}
      }
    }
  }
  
  enum Stage { EDGE_TAIL = 1, EDGE_HEAD, PARENT_NODES, END };

  Stage stage;
  const edge_data_s * edge_ptr, * edge_end;
  const node_tertiary_data_s * node_ptr, * node_end;
  std::vector<bool> processed_nodes;
  std::vector<int> parent_nodes;
  size_t num_nodes;
  int current_node;
};

#endif
