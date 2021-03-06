#ifndef _VISIBLENODEITERATOR_H_
#define _VISIBLENODEITERATOR_H_

#include <unordered_set>
#include <vector>

class ConstVisibleNodeIterator {
 public:
  ConstVisibleNodeIterator(const edge_data_s * _edge_ptr,
			   const edge_data_s * _edge_end,
			   const node_tertiary_data_s * _node_ptr,
			   const node_tertiary_data_s * _node_end,
			   size_t _num_nodes,
			   int _active_node_id)
    : stage(_edge_ptr < _edge_end ? EDGE_TAIL : END),
    edge_ptr(_edge_ptr),
    edge_end(_edge_end),
    node_ptr(_node_ptr),
    node_end(_node_end),
    num_nodes(_num_nodes),
    active_node_id(_active_node_id)
  {
    open_nodes.insert(-1);
    for (int p = active_node_id; p != -1; p = node_ptr[p].parent_node) {
      open_nodes.insert(p);
    }
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
    num_nodes(0),
    active_node_id(-1) {
  }

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
	  bool visible = true;
	  if (node_ptr + n < node_end) {
	    int p = node_ptr[n].parent_node;
	    if (p != -1) {
	      if (!open_nodes.count(p)) {
		visible = false;
	      }
	      parent_nodes.push_back(p);
	    }
	    if (visible) {
	      int l = node_ptr[n].group_leader;
	      if (l != -1) {
		processed_nodes[l] = false;
		leader_nodes.push_back(l);
	      }
	    }
	  }
	  if (visible) current_node = n;
	}
	stage = EDGE_HEAD;
      } else if (stage == EDGE_HEAD) {
	int n = edge_ptr->head;
	if (!processed_nodes[n]) {
	  processed_nodes[n] = true;
	  bool visible = true;
	  if (node_ptr + n < node_end) {
	    int p = node_ptr[n].parent_node;
	    if (p != -1) {
	      if (!open_nodes.count(p)) {
		visible = false;
	      }
	      parent_nodes.push_back(p);
	    }
	    if (visible) {
	      int l = node_ptr[n].group_leader;
	      if (l != -1) {
		processed_nodes[l] = false;
		leader_nodes.push_back(l);
	      }
	    }
	  }
	  if (visible) current_node = n;	  
	}
	edge_ptr++;
	if (edge_ptr != edge_end) {
	  stage = EDGE_TAIL;
	} else {
	  stage = PARENT_NODES;
	}
      } else if (stage == PARENT_NODES) {
	if (parent_nodes.empty()) {
	  stage = LEADER_NODES;
	} else {
	  int n = parent_nodes.back();
	  parent_nodes.pop_back();
	  if (!processed_nodes[n]) {
	    processed_nodes[n] = true;
	    bool visible = true;
	    if (node_ptr + n < node_end) {
	      int p = node_ptr[n].parent_node;
	      if (p != -1) {
		if (!open_nodes.count(p)) {
		  visible = false;
		}
		parent_nodes.push_back(p);
	      }
	      if (visible) {
		int l = node_ptr[n].group_leader;
		if (l != -1) {
		  processed_nodes[l] = false;
		  leader_nodes.push_back(l);
		}
	      }
	    }
	    if (visible) current_node = n;
	  }
	}
      } else if (stage == LEADER_NODES) {
	if (leader_nodes.empty()) {
	  stage = END;
	} else {
	  int n = leader_nodes.back();
	  leader_nodes.pop_back();
	  if (!processed_nodes[n]) {
	    processed_nodes[n] = true;
	    current_node = n;
	  }
	}
      }
    }
  }
  
  enum Stage { EDGE_TAIL = 1, EDGE_HEAD, PARENT_NODES, LEADER_NODES, END };

  Stage stage;
  const edge_data_s * edge_ptr, * edge_end;
  const node_tertiary_data_s * node_ptr, * node_end;
  std::vector<bool> processed_nodes;
  std::vector<int> parent_nodes;
  std::vector<int> leader_nodes;
  size_t num_nodes;
  int current_node;
  int active_node_id;
  std::unordered_set<int> open_nodes;
};

#endif
