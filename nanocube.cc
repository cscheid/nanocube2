#include "nanocube.hh"

namespace {
  
inline int get_bit(int value, int bit)
{
  return (value >> bit) & 1;
}

};

/******************************************************************************/
// Simple accessors

template <typename Summary>
inline int Nanocube<Summary>::get_summary_index(int node_index, int dim)
{
  // a null node should always return a null summary.
  if (node_index == -1) {
    return -1;
  }
  // otherwise, walk the "next" chain of pointers until we get to the summary table
  do {
    node_index = dims.at(dim).at(node_index);
    ++dim;
  } while (dim < dims.length);
  return node_index;
};

template <typename Summary>
inline pair<int, int> Nanocube<Summary>::get_children(int node_index, int dim)
{
  if (node_index == -1) {
    return make_pair(-1, -1);
  } else {
    return dims.at(dim).children.at(node_index);
  }
}

/******************************************************************************/

template <typename Summary>
set<int> Nanocube<Summary>::next_indices
(int root_index, int address, int dim)
{
  NCDim &nc_dim = dims[dim];
  set<int> result;
  int current_index = root_index;
  for (int i=0; i<width; ++i) {
    result.insert(nc_dim.next[current_index]);
    int bit = get_bit(address, width-i-1);
    if (bit) {
      current_index = nc_dim.children[current_index].second;
    } else {
      current_index = nc_dim.children[current_index].first;
    }
  }
  return result;
};
  

// insert_node returns a pair of node indices: the first element is
// the node index of the newly inserted node at the current dimension.
// the second element is the node index of the new summary

template <typename Summary>
pair<int, int> Nanocube<Summary>::insert_node
(Summary &summary, vector<int> &addresses, int dim, int current_node, int current_address, int current_bit)
{
  NCDim &nc_dim = dims.at(dim);
  int width = nc_dim.width;
  int this_summary_index = get_summary_index(current_node, dim);
  pair<int, int> recursion_result;
  if (current_bit == width) {
    recursion_result = insert_dim(summary, addresses, dim+1, dims.at(dim+1).at(nc_dim.next(current_node)), addresses.at(dim+1), 0);
  } else {
    int bit = get_bit(current_address, width-current_bit-1);
    int next_node;
    pair<int, int> children = nc_dim.get_children(current_node);
    if (bit) {
      next_node = children.second;
    } else {
      next_node = children.first;
    }
    recursion_result = insert_node(summary, addresses, dim, next_node, current_address, current_bit+1);

    // now do a case analysis on the number of children we had before the insertion.
    int result_node_index, result_summary_index;
    
    if (children.first == -1 && children.second == -1) {
      // we had no children, so we will now have precisely one child, and
      // our next is precisely the next of the recursion result (ie. sharing occurs).
      result_summary_index = recursion_result.second;
      result_node_index = nc_dim.children.length;
      
      pair<int, int> new_children = make_pair((!bit) ? result_node_index : children.first,
					      ( bit) ? result_node_index : children.second);
      int new_next = nc_dim.next.at(recursion_result.first);
      nc_dim.children.push_back(new_children);
      nc_dim.next.push_back(new_next);
    } else if (children.first != -1 && children.second != -1) {
      // we had two children. 

      // two children means we need a new summary, because no sharing is possible in this case.

      // if we inserted on the right, we need to get the summary on the left, and vice-versa
      int other_child = bit ? children.first : children.second;
      int other_summary_index = get_summary_index(other_child, dim);
      int new_summary = summaries.at(recursion_result.second) + summaries.at(other_summary_index);
      new_summary_index = summaries.length;
      summaries.insert_summary(new_summary);
      
      result_node_index = nc_dim.children.length;
      pair<int, int> new_children = make_pair((!bit) ? result_node_index : children.first,
					      ( bit) ? result_node_index : children.second);
      int new_next = nc_dim.next.at(recursion_result.first);

      nc_dim.children.push_back(new_children);
      nc_dim.next.push_back(new_next);
      return make_pair(new_node, new_summary_index);
    } else if ((children.first == -1 && children.second != -1) ||
	       (children.first != -1 && children.second == 1)) {
      // we had exactly one child.
      
      // This means all of our previous additions were to the same side of the tree, which means
      // our own tree node can be reused.

      // let's mutate the data structure now. DANGER, WILL ROBINSON.
      pair<int, int> &children_ref = nc_dim.children.at(current_node);
      nc_dim.next.at(current_node) = nc_dim.next.at(recursion_result.first);
      if (children_ref.first != -1) {
	assert(children_ref.second == -1);
	children_ref.first  = recursion_result.first;
      } else {
	assert(children_ref.second != -1);
	children_ref.second = recursion_result.first; // NB: this is *not* recursion_result.second.
      }
      
      // since we mutated the data structure in-place, we return,
      // as the new node, our *current* node.
      result_node_index = current_node;
      result_summary_index = recursion_result.second;
    }
    return make_pair(result_node_index, result_summary_index);
  }
}

template <typename Summary>
pair<int, int> Nanocube::insert_dim
(Summary &summary, vector<int> &addresses, int root_index, int dim)
{
  if (dim == dims.length) {
    // FIXME handle summaries
  } else {
    int current_address = addresses[dim];
    
    return insert_node(summary, addresses, dim, root_index, current_address, 0);
    
    set<int> next_is = next_indices(root_index, current_address, dim);
    int current_index = root_index;
    int width = nc_dim[dim].width;

    for (int i=0; i<width; ++i) {
      int new_index = insert(summary, addresses, nc_dim[dim].next[current_index], dim+1);
      // THIS IS NOWHERE FINISHED;
    }
  }
}
 
