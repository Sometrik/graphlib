#ifndef _VISIBLENODEITERATOR_H_
#define _VISIBLENODEITERATOR_H_

class ConstVisibleNodeIterator {
 public:
  ConstVisibleNodeIterator(const edge_data_s * _edge_ptr,
			   const edge_data_s * _edge_end,
			   const node_tertiary_data_s * _node_data,
			   size_t _num_nodes)
    : stage(EDGE_TAIL),
    edge_ptr(_edge_ptr),
    edge_end(_edge_end),
    node_data(_node_data),
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
    node_data(0),
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
	  if (node_data[n].parent_node != -1) parent_nodes.push_back(node_data[n].parent_node);
	  current_node = n;
	}
	stage = EDGE_HEAD;
      } else if (stage == EDGE_HEAD) {
	int n = edge_ptr->head;
	if (!processed_nodes[n]) {
	  processed_nodes[n] = true;
	  if (node_data[n].parent_node != -1) parent_nodes.push_back(node_data[n].parent_node);
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
	    if (node_data[n].parent_node != -1) parent_nodes.push_back(node_data[n].parent_node);
	    current_node = n;
	  }
	}
      }
    }
  }
  
  enum Stage { EDGE_TAIL = 1, EDGE_HEAD, PARENT_NODES, END };

  Stage stage;
  const edge_data_s * edge_ptr, * edge_end;
  const node_tertiary_data_s * node_data;
  std::vector<bool> processed_nodes;
  std::vector<int> parent_nodes;
  size_t num_nodes;
  int current_node;
};

#endif
