// Copyright 2016 Arizona Board of Regents. See README.md and LICENSE for more.

#include <algorithm>
#include <iostream>

namespace {
  
inline int get_bit(int64_t value, int bit)
{
  return (value >> bit) & 1;
}

};

#define COUT (cout << __FILE__ << ":" << __LINE__ << " - ")
#define TRACE (cout << __FILE__ << ":" << __LINE__)
#define var(x) #x << ":" << x << " "

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
  while (dim < dims.size()) {
    node_index = dims.at(dim).at(node_index).next;
    ++dim;
  }
  return node_index;
};

template <typename Summary>
inline NCDimNode Nanocube<Summary>::get_children(int node_index, int dim)
{
  if (node_index == -1) {
    return NCDimNode {-1, -1, -1};
  } else {
    return dims.at(dim).at(node_index);
  }
}

/******************************************************************************/

template <typename Summary>
inline int Nanocube<Summary>::make_node_ref(int node_index, int dim)
{
  if (node_index == -1) {
    return 0;
  }
  if (dim == dims.size()) {
    return summaries.make_ref(node_index);
  } else {
    assert(node_index < dims[dim].size());
    return dims[dim].nodes.make_ref(node_index);
  }
}

template <typename Summary>
inline void Nanocube<Summary>::release_node_ref(int node_index, int dim)
{
  vector<pair<int, int> > stack;
  stack.push_back(make_pair(node_index, dim));
  
  if (node_index == -1) {
    return;
  }
  
  while (stack.size()) {
    pair<int, int> top = stack.back();
    int dim = top.second, node_index = top.first;
    stack.pop_back();
    if (dim == dims.size()) {
      summaries.release_ref(node_index);
    } else {
      NCDimNode &node = dims.at(dim).at(node_index);
      int new_ref_count = dims.at(dim).nodes.release_ref(node_index);

      if (new_ref_count == 0) {
        if (node.left != -1) {
          stack.push_back(make_pair(node.left, dim));
        }
        if (node.right != -1) {
          stack.push_back(make_pair(node.right, dim));
        }
        if (node.next != -1) {
          stack.push_back(make_pair(node.next, dim+1));
        }
        node.left = -1;
        node.right = -1;
        node.next = -1;
      }
    }
  }
  
  // if (dim == dims.size()) {
  //   // this is a summaries ref.
  //   return summaries.release_ref(node_index);
  // } else {
  //   // this is an internal ref
  //   assert(node_index < dims.at(dim).size());
  //   NCDimNode &node = dims.at(dim).at(node_index);
  //   int new_ref_count = dims.at(dim).nodes.release_ref(node_index);

  //   if (new_ref_count == 0) {
  //     release_node_ref(node.left, dim);
  //     release_node_ref(node.right, dim);
  //     release_node_ref(node.next, dim+1);
  //     node.left = -1;
  //     node.right = -1;
  //     node.next = -1;
  //   }
  //   return new_ref_count;
  // }
}

template <typename Summary>
inline int Nanocube<Summary>::get_node_ref(int node_index, int dim)
{
  return dims.at(dim).nodes.ref_counts.at(node_index);
}

template <typename Summary>
pair<int, int> Nanocube<Summary>::insert_fresh_node
(const Summary &summary, const vector<int64_t> &addresses, int dim, int bit)
{
  assert(dims.size() == addresses.size());
  
  if (dim == dims.size()) {
    int new_ref = summaries.insert(summary);
    return make_pair(new_ref, new_ref);
  } else {
    NCDim &nc_dim = dims.at(dim);
    int width = nc_dim.width;
    if (bit == width) {
      pair<int, int> next_dim_result = insert_fresh_node(summary, addresses, dim+1, 0);
      NCDimNode new_node_at_current_dim { -1, -1, next_dim_result.first };
      make_node_ref(next_dim_result.first, dim+1);
      int new_index = nc_dim.nodes.insert(new_node_at_current_dim);
      return make_pair(new_index, next_dim_result.second);
    } else {
      int refinement_node;
      int where_to_insert = get_bit(addresses[dim], width-bit-1);
      pair<int, int> recursion_result = insert_fresh_node(summary, addresses, dim, bit+1);
      int next = nc_dim.at(recursion_result.first).next;
      NCDimNode new_refinement_node(
          (!where_to_insert) ? recursion_result.first : -1,
          ( where_to_insert) ? recursion_result.first : -1,
          next);
      make_node_ref(next, dim+1);
      make_node_ref(recursion_result.first, dim);
      int new_index = nc_dim.nodes.insert(new_refinement_node);
      return make_pair(new_index, recursion_result.second);
    }
  }
}

// insert_node returns a pair of indices: the first element is the
// index of the newly inserted node at $dim$.  the second element is
// the index where $summary$ was appended to.  note that this is not
// guaranteed to be thesame as $summary$ itself, particularly in the
// case where two different summaries get added to exactly the same
// address (In other words, when a nanocube cannot resolve two
// different data points)
/*
template <typename Summary>
pair<int, int> Nanocube<Summary>::insert_node
(const Summary &summary, const vector<int64_t> &addresses, int current_node_index, int dim, int bit,
 SummaryCache &summary_cache)
{
  // debug_out << "Entering index:" << current_node_index
  //      << " dim:" << dim
  //      << " depth:" << bit << endl;
  assert(dims.size() == addresses.size());
  if (dim == dims.size()) {
    // debug_out << "Will insert summary" << endl;
    // we're done inserting things on the nanocube itself.
    // all we need to do is add the summary to the summary vector

    // for the last dimension, the summary index is both the index of the
    // inserted node, and the summary index itself.
    pair<int, int> k = make_pair(current_node_index, -1);

    int new_ref;
    if (current_node_index == -1) {
        new_ref = summaries.insert(summary);
    } else {
        const Summary &other_summary = summaries.values.at(current_node_index);
        Summary new_summary = other_summary + summary;
        new_ref = summaries.insert(new_summary);
    }
    return make_pair(new_ref, new_ref);
  }
  assert(dim < dims.size());
  
  NCDim &nc_dim = dims.at(dim);
  NCDimNode current_node = get_children(current_node_index, dim);
  int64_t current_address = addresses.at(dim);
  int width = nc_dim.width;
  pair<int, int> recursion_result;
  if (bit == width) {
    // debug_out << "At bottom of refinement tree. index:" << current_node_index << " dim:" << dim << " depth:" << bit << endl;
    // we're at the bottom of the refinement tree.
    
    assert(current_node.left == current_node.right && current_node.left == -1);

    // recurse into the next dimension
    // debug_out << "recursing into next dimension" << endl;
    recursion_result = insert_node(summary, addresses, current_node.next, dim+1, 0, summary_cache);
    // debug_out << "out of next-dim recursion" << endl;

    if (current_node_index == -1) {
      // we're inserting into a new address.
      NCDimNode new_node_at_current_dim { -1, -1, recursion_result.first };
      make_node_ref(recursion_result.first, dim+1);
      int new_index = nc_dim.nodes.insert(new_node_at_current_dim);
      // debug_out << "Inserting into a new address (" << new_index << ": -1 -1 " << recursion_result.first << ")" << endl;
      return make_pair(new_index, recursion_result.second);
    } else {
      // we're inserting into an existing address.
      // debug_out << "Inserting into an existing address" << endl;
      assert(current_node.next != -1);

      // but we're at the bottom of the refinement tree.
      // These two conditions, together, mean that we are simply replacing the next
      // reference at the current node.

      set_next_node_ref(current_node_index, dim, recursion_result.first);
      // release_node_ref(current_node.next, dim+1);
      // nc_dim.at(current_node_index).next = recursion_result.first;
      // dims.at(dim+1).nodes.make_ref(recursion_result.first);
      
      return make_pair(current_node_index, recursion_result.second);
    }
  } else {
    // we're in the interior of the refinement tree
    // debug_out << "Inside the refinement tree" << endl;
    
    int refinement_node;
    int where_to_insert = get_bit(current_address, width-bit-1);
    // debug_out << "bit " << bit << " of " << current_address << " is " << where_to_insert << endl;
    if (where_to_insert) {
      // debug_out << "Recursing right" << endl;
      refinement_node = current_node.right;
    } else {
      // debug_out << "Recursing left" << endl;
      refinement_node = current_node.left;
    }
    
    // recurse down the refinement tree
    // debug_out << "recurse down the refinement tree" << endl;
    recursion_result = insert_node(summary, addresses, refinement_node, dim, bit+1, summary_cache);
    // debug_out << "out of ref-tree recursion" << endl;
    NCDimNode &recursion_result_node = nc_dim.at(recursion_result.first);

    // now we do a case analysis on the number of children we had before the insertion.
    if (current_node.left == -1 && current_node.right == -1) {
      // debug_out << "we had no children,";
      
      // we had no children, but we were an interior node.
      // this means we're actually a ghost node (because all addresses have the same depth,
      // and so all leaves must be at the bottom of the tree)
      // as a result, we will create a new node now to represent the current address.
      
      // In addition, our next is precisely the next of the recursion
      // result (so sharing occurs).


      NCDimNode node_we_will_insert(
         (!where_to_insert) ? recursion_result.first : -1,
         ( where_to_insert) ? recursion_result.first : -1,
         recursion_result_node.next);
      // we need to update reference counts
      make_node_ref(recursion_result_node.next, dim+1);
      make_node_ref(recursion_result.first, dim);
      int new_index = nc_dim.nodes.insert(node_we_will_insert);
      // debug_out << " inserting (" << new_index << ": " << node_we_will_insert.left << " " << node_we_will_insert.right << " " << node_we_will_insert.next << ")" << endl;
      return make_pair(new_index, recursion_result.second);
    } else if (current_node.left != -1 && current_node.right != -1) {
      // debug_out << "node " << current_node_index << " had two children" << endl;
      // we had two children. 

      // // two children means we need a new summary, because no sharing is possible in this case.
      // // if we inserted on the right, we need to get the summary on the left, and vice-versa
      // int other_child = bit ? current_node.left : current_node.right;
      // int other_summary_index = get_summary_index(other_child, dim);
      // T new_summary = summaries.at(recursion_result.second) + summaries.at(other_summary_index);
      // int new_summary_index = summaries.insert(new_summary);

      if (where_to_insert) {
        // insertion happened on the right, need to update right child
        // if recursion_result.first is equal to current_node.right, we
        // don't need to change references, because we're still going to refer
        // to the same node.
        set_right_node_ref(current_node_index, dim, recursion_result.first);
      } else {
        // similarly to left child
        set_left_node_ref(current_node_index, dim, recursion_result.first);
      }

      pair<int, int> fresh_new_node = insert_node(summary, addresses, -1, dim+1, 0, summary_cache);
      make_node_ref(fresh_new_node.first, dim+1);
      pair<int, int> merge_result = merge(current_node.next,
                                          fresh_new_node.first,
                                          dim+1, summary_cache);
      set_next_node_ref(current_node_index, dim, merge_result.first);
      release_node_ref(fresh_new_node.first, dim+1);
      return make_pair(current_node_index, merge_result.second);
      
          
      // now we must update the next pointer as well, and that requires another
      // recursive insertion, because no sharing exists.
      // COUT << "Must merge next" << endl;
      // pair<int, int> merge_result = merge(dims.at(dim).at(current_node.left).next,
      //                                     dims.at(dim).at(current_node.right).next,
      //                                     dim+1, summary_cache);
      // set_next_node_ref(current_node_index, dim, merge_result.first);
      // return make_pair(current_node_index, merge_result.second);
      
    } else {
      assert((current_node.left == -1 && current_node.right != -1) ||
             (current_node.left != -1 && current_node.right == -1));
      // debug_out << "we had one child " << current_node.left << " " << current_node.right << endl;
      // we had exactly one child.
      
      if (where_to_insert) {
        // insertion happened on the right, need to update right child
        // if recursion_result.first is equal to current_node.right, we
        // don't need to change references, because we're still going to refer
        // to the same node.
        set_right_node_ref(current_node_index, dim, recursion_result.first);
      } else {
        // similarly to left child
        set_left_node_ref(current_node_index, dim, recursion_result.first);
      }

      // debug_out << "Before merge var init" << endl;
      // debug_out << "  We want to merge dim:" << dim
      //           << " left:" << current_node.left
      //           << " right:" << current_node.right << endl;
      
      // after the insertion, 
      // we need a merge of the nodes.
      // if (nc_dim.at(current_node_index).left != -1 && nc_dim.at(current_node_index).right != -1) {
      // "merging"

      const NCDimNode &current = nc_dim.at(current_node_index);
      NCDimNode new_children = get_children(current_node_index, dim);
      
      int left_next = get_children(current.left, dim).next,
         right_next = get_children(current.right, dim).next;
      // debug_out << "Merging left:" << left_next << " right:" << right_next << " dim:" << dim+1 << endl;

      // COUT << "must merge next " << endl;
      pair<int, int> merge_result = merge(left_next, right_next, dim+1, summary_cache);

      // debug_out << "Out of merge" << endl;

      // debug_out << "mutating node ("
      //           << current.left << ","
      //           << current.right << ","
      //           << current.next
      //           << "), setting its next to " << merge_result.first << endl;
      set_next_node_ref(current_node_index, dim, merge_result.first);
      return make_pair(current_node_index, merge_result.second);
    }
  }
}
*/

template <typename Summary>
pair<int, int> Nanocube<Summary>::merge
(int node1, int node2, int dim, SummaryCache &summary_cache)
{
  if (node1 == -1)
    return make_pair(node2, get_summary_index(node2, dim));
  if (node2== -1)
    return make_pair(node1, get_summary_index(node1, dim));

  if (dim == dims.size()) {
    auto f = summary_cache.find(make_pair(node1, node2));
    int new_summary_index;
    if (f == summary_cache.end()) {
      const Summary &node1_summary = summaries.values.at(node1),
                   &node2_summary = summaries.values.at(node2);
      Summary new_summary = node1_summary + node2_summary;
      new_summary_index = summaries.insert(new_summary);
      summary_cache[make_pair(node1, node2)] = new_summary_index;
    } else {
      new_summary_index = f->second;
    }
    return make_pair(new_summary_index, new_summary_index);
  } else {
    NCDimNode node1_node = dims.at(dim).at(node1),
             node2_node = dims.at(dim).at(node2);
    pair<int, int> left_merge = merge(node1_node.left,  node2_node.left,  dim,   summary_cache),
                  right_merge = merge(node1_node.right, node2_node.right, dim,   summary_cache);
    pair<int, int> next_merge;

    int left_index = left_merge.first,
       right_index = right_merge.first;
    
    if        (left_index == -1 && right_index == -1) {
      next_merge = merge(node1_node.next, node2_node.next, dim+1, summary_cache);
    } else if (left_index == -1 && right_index != -1) {
      next_merge = make_pair(dims.at(dim).at(right_index).next, right_merge.second);
    } else if (left_index != -1 && right_index == -1) {
      next_merge = make_pair(dims.at(dim).at(left_index).next, left_merge.second);
    } else if (left_index != -1 && right_index != -1) {
      next_merge = merge(node1_node.next, node2_node.next, dim+1, summary_cache);
    }
    
    NCDimNode new_node(left_merge.first, right_merge.first, next_merge.first);
    make_node_ref(left_merge.first, dim);
    make_node_ref(right_merge.first, dim);
    make_node_ref(next_merge.first, dim+1);
    
    int new_node_index = dims.at(dim).nodes.insert(new_node);
    return make_pair(new_node_index, next_merge.second);
  }
}

template <typename Summary>
void Nanocube<Summary>::insert
(const Summary &summary, const vector<int64_t> &addresses)
{
  SummaryCache summary_cache;
  pair<int, int> fresh_node = insert_fresh_node(summary, addresses, 0, 0);

  make_node_ref(fresh_node.first, 0);
  pair<int, int> result = merge(base_root, fresh_node.first, 0, summary_cache);
  make_node_ref(result.first, 0);
  release_node_ref(base_root, 0);
  release_node_ref(fresh_node.first, 0);
  base_root = result.first;
}

template <typename Summary>
void Nanocube<Summary>::new_insert
(const Summary &summary, const vector<int64_t> &addresses)
{
  // only happens at the first time
  if (base_root == -1) {
    pair<int, int> insert_result = insert_fresh_node(summary, addresses, 0, 0);
    base_root = insert_result.first;
    make_node_ref(base_root, 0);
    return;
  }

  // traverse tree, if a node has two child or is a leaf node
  // put into queue until the position to insert
  //      if the node exists, do the same thing, else insert fresh node
  // Get one node from queue, if it is the n-1 dimension, do the update
  // else do the same traversal for node.next
  stack<NCMutantNode> mutant_nodes; // <index, dim>
  mutant_nodes.push(NCMutantNode(base_root, -1, base_root));

  // summary cache 
  // key: summary index
  // value: new summary index
  map<int, int> cache;
  
  while(mutant_nodes.size() != 0) {
    NCMutantNode current = mutant_nodes.top();
    //cout << "current: " << current.first << ", " << current.second << endl;
    mutant_nodes.pop();

    assert(current.dim<= dims.size());
    assert(current.index != -1);

    if(current.dim == dims.size()-1) {
      // if current is n-1 dimension, do the update
      NCDimNode &node = dims.at(current.dim).at(current.index);
      // if node.next already in cache, then just point node.next to
      // existing new summary
      auto f = cache.find(node.next);
      //cout << node.next << endl;
      if (f == cache.end()) {
        const Summary &other_summary = summaries.values.at(node.next);
        Summary new_summary = other_summary + summary;
        int new_ref = summaries.insert(new_summary);

        cache[node.next] = new_ref;
        this->summaries.make_ref(new_ref);
        this->summaries.release_ref(node.next);

        node.next = new_ref;
      } else {
        this->summaries.release_ref(node.next);
        node.next = f->second;
        this->summaries.make_ref(node.next);
      }

    } else {
      // else keep updating for the next dimension

      // base_root
      if ( current.dim == -1 ) {
        update_tree(summary, 
                    addresses,
                    current.dim+1,
                    base_root,
                    mutant_nodes);
      } else {
        NCDimNode &current_node = dims.at(current.dim).at(current.index);
        // if current is a leaf node, or
        // if current->next only has one reference, 
        // then no need to make a copy, just update in place
        if ( (current_node.left == -1 && current_node.right == -1) ||
             (get_node_ref(current_node.next, current.dim+1) == 1) ) {
          update_tree(summary, 
                      addresses,
                      current.dim+1,
                      current_node.next,
                      mutant_nodes);
        } else {
          // current->next is shared by multiple nodes, 
          // make a copy and re-point all parents of current nodes to the new_next
          //cout << "copy the next of node: " << current.first << ", " << current.second << endl;
          auto copy = copy_tree(addresses, current_node.next, current.dim+1);
          release_node_ref(current_node.next, current.dim+1);
          make_node_ref(copy.first, copy.second);

          update_tree(summary, 
                      addresses,
                      current.dim+1,
                      copy.first,
                      mutant_nodes);
          // update the 'next' pointer of all parents that used to
          // point to the same 'old_next'
          // TODO currently just traverse the tree from the root to current node
          // maybe we can optimise this in the future
          //dims.at(current.dim).at(current.index).next = copy.first;
          // update_next_pointer(dim, root_index, current_index, old_next, new_next);
          update_next_pointer( 
            addresses, current.dim, current.root_index, current.index, 
            dims.at(current.dim).at(current.index).next,
            copy.first
          );
        }
      }
    }
  }
}


// update the next pointers for nodes along the path from 
// Nodes[root_index] to Nodes[stop_index]
// only update the node that node.next == old_next
template <typename Summary>
void Nanocube<Summary>::update_next_pointer
(const vector<int64_t> &addresses, int dim, int root_index, int stop_index,
 int old_next, int new_next)
{
  //cout << "update next pointer: " << endl;
  //cout << "dim: " << dim << ", root:" << root_index << ", stop:" << stop_index;
  //cout << ", old_next: " << old_next << ", new_next: " << new_next << endl;
  int depth = 0;
  int current = root_index;
  int dimWidth = dims.at(dim).width;

  while(depth <= dimWidth) {
    NCDimNode &current_node = dims.at(dim).at(current);
    if (current_node.next == old_next) {
      current_node.next = new_next;
    }

    if (current == stop_index) {
      // don't update the nodes after Nodes[stop_index]
      break;
    } else {
      // refine 
      depth += 1;
      int where_to_insert = get_bit(addresses[dim], dimWidth-depth);
      current = (!where_to_insert) ? current_node.left : current_node.right;
    }
  }
}


// from the root index to traverse the tree
// find all nodes that need to be updated
// insert a fresh node if the desired node doesn't exist
template <typename Summary>
void Nanocube<Summary>::update_tree
(const Summary &summary, const vector<int64_t> &addresses, const int dim, 
 const int node_index, stack<NCMutantNode> &mutant_nodes)
{
  int depth = 0;
  int current = node_index;
  int previous = -1;
  int dimWidth = dims.at(dim).width;
  //cout << "dim depth: " << dimWidth << endl;
  while(depth <= dimWidth) {
    //cout << "begin refinement\n";
    if (current == -1) {
      // insert fresh node here
      //cout << "insert fresh node: " << addresses[dim] << ", " << dim << ", " << depth << endl;
      pair<int, int> insert_result = 
        insert_fresh_node(summary, addresses, dim, depth);
      make_node_ref(insert_result.first, dim);
      int where_to_insert = get_bit(addresses[dim], dimWidth-depth);
      NCDimNode &parent_node = dims.at(dim).at(previous);
      (!where_to_insert) ? parent_node.left = insert_result.first : parent_node.right = insert_result.first;

      // after insertion, if the parent node is a full node now, put it in
      // mutant_nodes because it should be mutated
      // don't need to include it if this is the last dimension
      if (dim < dims.size()-1 && 
          parent_node.right != -1 && 
          parent_node.left != -1) {
        mutant_nodes.push(NCMutantNode(previous, dim, node_index));
      }

      break; // if it's an insert, we can stop now
    }

    NCDimNode &node = dims.at(dim).at(current);

    // update and insert at the n-1 dimension should mutate the whole tree
    // the nodes point to the same summary need to point to the new summary
    if( (dim == dims.size()-1) ||  // TODO find better solution
        (node.left != -1 && node.right != -1) || 
        (depth == dimWidth) ) {
      //cout << "update node: " << current << ", " << dim << endl;
      mutant_nodes.push(NCMutantNode(current, dim, node_index));

      if (depth == dimWidth) {
        break;
      }
    }

    // refine 
    depth += 1;
    int where_to_insert = get_bit(addresses[dim], dimWidth-depth);
    //cout << "depth: " << depth << endl;
    //cout << "bit: " << dimWidth-depth << endl;
    //cout << "where_to_insert: " << where_to_insert << endl;
    previous = current;
    current = (!where_to_insert) ? node.left : node.right;
  }
}

template <typename Summary>
pair<int, int> Nanocube<Summary>::copy_tree
(const vector<int64_t> &addresses, int index, int dim)
{
  //cout << "copy tree root at: " << index << ", dim: " << dim << endl;
  int depth = 0;
  int current = index;
  int previous = -1;
  int new_root = -1;
  int dimWidth = dims.at(dim).width;
  stack<int> path;

  while(depth <= dimWidth) {
    if (current == -1) {
      break;
    }

    NCDimNode copied_node(dims.at(dim).at(current));
    int new_index = dims.at(dim).nodes.insert(copied_node);
    // since we have a copy, we need to increase the referece for each copied node
    make_node_ref(copied_node.next, dim+1);
    //cout << "source: " << index <<": " << copied_node.left << ", " << copied_node.right << ", " << copied_node.next << endl;
    //cout << new_index << "->";
    if (previous == -1) {
      // this means the new node is a copy of old root
      new_root = new_index;
      previous = new_index;
    } else {
      // only reference here, don't make reference of root node
      make_node_ref(new_index, dim);
      NCDimNode &parent = dims.at(dim).at(previous);
      int where_to_insert = get_bit(addresses[dim], dimWidth-depth);
      (!where_to_insert) ? parent.left = new_index : parent.right = new_index;
      previous = new_index;
    }

    // refine 
    depth += 1;
    int where_to_insert = get_bit(addresses[dim], dimWidth-depth);
    NCDimNode &node = dims.at(dim).at(current);
    current = (!where_to_insert) ? node.left : node.right;
  }
  //cout << endl;

  return make_pair(new_root, dim);
}

template <typename Summary>
void Nanocube<Summary>::compact() {
  auto previous_map = summaries.compact();
  for (auto rb = dims.rbegin(), re = dims.rend(); re != rb; ++rb) {
    assert(rb->nodes.values.size() == rb->nodes.ref_counts.size());
    size_t sz = rb->nodes.values.size();
    auto e = previous_map.end();
    for (int i=0; i<sz; ++i) {
      auto f = previous_map.find(rb->nodes.values[i].next);
      if (f != e) {
        rb->nodes.values[i].next = f->second;
      }
    }
    previous_map = rb->nodes.compact();
    e = previous_map.end();
    for (int i=0; i<sz; ++i) {
      auto f = previous_map.find(rb->nodes.values[i].left);
      if (f != e) {
        rb->nodes.values[i].left = f->second;
      }
      f = previous_map.find(rb->nodes.values[i].right);
      if (f != e) {
        rb->nodes.values[i].right = f->second;
      }
    }
  }
  auto f = previous_map.find(base_root);
  if (f != previous_map.end()) {
    base_root = f->second;
  }
}

template <typename Summary>
struct SummaryComparator
{
  std::vector<Summary> &vec_;
  explicit SummaryComparator(std::vector<Summary> &vec): vec_(vec) {};
  
  bool operator()(int v1, int v2) {
    return vec_[v1] < vec_[v2];
  }
};

// find_uniques assumes a sorted vector
template <typename Summary>
std::vector<int> find_uniques(std::vector<Summary> &summaries)
{
  if (summaries.size() < 1) {
    return vector<int>();
  }
  vector<int> result { 0 };
  int previous = 0, current = 1;
  while (current < summaries.size()) {
    if (summaries[previous] == summaries[current]) {
      result.push_back(previous);
    } else {
      result.push_back(current);
      previous = current;
    }
    ++current;
  }
  return result;
}

template <typename Summary>
void Nanocube<Summary>::content_compact()
{
  compact();
  assert(summaries.free_list.size() == 0);
  std::vector<int> summary_indices;
  for (int i=0; i<summaries.values.size(); ++i) {
    summary_indices.push_back(i);
  }

  sort(summary_indices.begin(), summary_indices.end(), SummaryComparator<Summary>(summaries.values));
  
  vector<Summary> old_summaries = summaries.values;
  vector<int> old_refcounts = summaries.ref_counts;
  for (int i=0; i<summaries.values.size(); ++i) {
    summaries.values.at(i) = old_summaries.at(summary_indices.at(i));
    summaries.ref_counts.at(i) = old_refcounts.at(summary_indices.at(i));
  }

  vector<int> summary_indices_inv(summary_indices.size());
  for (int i=0; i<summary_indices.size(); ++i) {
    summary_indices_inv[summary_indices[i]] = i;
  }

  // for now, only compact the dim that points to the summaries
  for (int i=0; i<dims.back().nodes.values.size(); ++i) {
    NCDimNode &node = dims.back().nodes.values.at(i);
    if (node.next != summary_indices_inv.at(node.next)) {
      node.next = summary_indices_inv.at(node.next);
    }
  }
  vector<int> uniques = find_uniques(summaries.values);
  for (int i=0; i<dims.back().nodes.values.size(); ++i) {
    NCDimNode &node = dims.back().nodes.values.at(i);
    set_next_node_ref(i, dims.size()-1, uniques[node.next]);
  }
  compact();
}

template <typename Summary>
Nanocube<Summary>::Nanocube(const vector<int> &widths, bool debug): unopened(), debug_out(debug?cout:unopened) {
  for (int i=0; i<widths.size(); ++i) {
    NCDim ncd;
    ncd.width = widths[i];
    dims.push_back(ncd);
  }
  base_root = -1;
}

/******************************************************************************/

template <typename Summary>
void Nanocube<Summary>::report_size() const
{
  cout << "Summary counts: " << summaries.values.size() << endl;
  cout << "Dimension counts:";
  for (size_t i=0; i<dims.size(); ++i) {
    cout << " " << dims.at(i).size();
  }
  cout << endl;
}

template <typename Summary>
void Nanocube<Summary>::dump_internals(bool force_print)
{
  ostream &out = (force_print ? cerr : debug_out);
  out << "Root node:" << base_root << endl;
  for (int i=0; i<dims.size(); ++i) {
    out << "Dimension " << i << endl;
    out << "\ti\trc\tleft\tright\tnext" << endl;
    RefCountedVec<NCDimNode> &vec = dims.at(i).nodes;
    for (int j=0; j<vec.values.size(); ++j) {
      out << "\t" << j
          << "\t" << vec.ref_counts.at(j)
           << "\t" << vec.values.at(j).left
           << "\t" << vec.values.at(j).right
           << "\t" << vec.values.at(j).next << endl;
    }
    out << "Free list:" << endl;
    for (int j=0; j<vec.free_list.size(); ++j) {
      out << "\t" << vec.free_list[j] << endl;
    }
    out << endl;
  }
  out << "Summaries" << endl;
  out << "\ti\trc\tvalue" << endl;
  for (int i=0; i<summaries.values.size(); ++i) {
    out << "\t" << i
        << "\t" << summaries.ref_counts.at(i)
         << "\t" << summaries.values.at(i) << endl;
  }
  out << "Free list:" << endl;
  for (int j=0; j<summaries.free_list.size(); ++j) {
    out << "\t" << summaries.free_list[j] << endl;
  }
}

/******************************************************************************/

template <typename Summary>
bool Nanocube<Summary>::validate_refcounts()
{
  bool kill = false;
  vector<vector<int> > ref_counts;
  for (int i=0; i<dims.size(); ++i) {
    NCDim &dim = dims.at(i);
    ref_counts.push_back(vector<int>(dim.size(), 0));
  }
  ref_counts.push_back(vector<int>(summaries.values.size(), 0));
  ref_counts[0][base_root]++;
  for (int i=0; i<ref_counts.size()-1; ++i) {
    NCDim &dim = dims.at(i);
    for (int j=0; j<dim.size(); ++j) {
      NCDimNode &node = dim.nodes.values.at(j);
      if (node.left != -1) {
        ref_counts.at(i).at(node.left)++;
      }
      if (node.right != -1) {
        ref_counts.at(i).at(node.right)++;
      }
      if (node.next != -1) {
        ref_counts.at(i+1).at(node.next)++;
      }
    }
    for (int j=0; j<dim.size(); ++j) {
      if (dim.nodes.ref_counts.at(j) != ref_counts.at(i).at(j)) {
        cout << "MISMATCH dim:" << i << " index:" << j << endl;
        cout << "  my computation: " << ref_counts.at(i).at(j) << endl;
        cout << "  value stored: " << dim.nodes.ref_counts.at(j) << endl;
        kill = true;
      }
    }
  }
  for (int j=0; j<summaries.ref_counts.size(); ++j) {
    if (summaries.ref_counts.at(j) != ref_counts.back().at(j)) {
        cout << "SUMMARY MISMATCH index:" << j << endl;
        cout << "  my computation: " << ref_counts.back().at(j) << endl;
        cout << "  value stored: " << summaries.ref_counts.at(j) << endl;
        kill = true;
    }
  }
  return !kill;
}

/******************************************************************************/

template <typename Summary>
Nanocube<Summary>::Nanocube(const Nanocube<Summary> &other):
    base_root(other.base_root),
    dims(other.dims),
    summaries(other.summaries),
    unopened(),
    debug_out(other.debug_out)
{}

template <typename Summary>
inline void Nanocube<Summary>::set_left_node_ref(int node_index, int dim, int value)
{
  int to_release = dims.at(dim).at(node_index).left;
  dims.at(dim).at(node_index).left = value;
  make_node_ref(value, dim);
  release_node_ref(to_release, dim);
}

template <typename Summary>
inline void Nanocube<Summary>::set_right_node_ref(int node_index, int dim, int value)
{
  int to_release = dims.at(dim).at(node_index).right;
  dims.at(dim).at(node_index).right = value;
  make_node_ref(value, dim);
  release_node_ref(to_release, dim);
}

template <typename Summary>
inline void Nanocube<Summary>::set_next_node_ref(int node_index, int dim, int value)
{
  int to_release = dims.at(dim).at(node_index).next;
  dims.at(dim).at(node_index).next = value;
  make_node_ref(value, dim+1);
  release_node_ref(to_release, dim+1);
}

/******************************************************************************/

// NB: this assumes that T doesn't has pointers inside it (or if it does,
// you'll have to figure them out by yourself.)
template <typename T>
void write_vector_to_binary_stream(std::ostream &os, const std::vector<T> &v)
{
  int sz = v.size();
  os.write((char*) &sz, sizeof(int));
  os.write((char*) &(v[0]), v.size() * sizeof(T));
}

template <typename Summary>
void Nanocube<Summary>::write_to_binary_stream(std::ostream &os)
{
  os.write((char*) &base_root, sizeof(int));
  int d = dims.size();
  os.write((char*) &d, sizeof(int));

  for (size_t i=0; i<dims.size(); ++i) {
    int w = dims[i].width;
    os.write((char *) &w, sizeof(int));
    write_vector_to_binary_stream(os, dims[i].nodes.values);
    write_vector_to_binary_stream(os, dims[i].nodes.ref_counts);
    write_vector_to_binary_stream(os, dims[i].nodes.free_list);
  }

  write_vector_to_binary_stream(os, summaries.values);
  write_vector_to_binary_stream(os, summaries.ref_counts);
  write_vector_to_binary_stream(os, summaries.free_list);
}

/******************************************************************************/

/* Local Variables:  */
/* mode: c++         */
/* End:              */
