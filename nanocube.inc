// Copyright 2016 Arizona Board of Regents. See README.md and LICENSE for more.

#include <algorithm>
#include <iostream>

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
int Nanocube<Summary>::make_node_ref(int node_index, int dim)
{
  if (node_index == -1) {
    return 0;
  }
  if (dim == dims.size()) {
    return summaries.make_ref(node_index);
  } else {
    assert(node_index < dims.at(dim).size());
    return dims.at(dim).nodes.make_ref(node_index);
  }
}

template <typename Summary>
int Nanocube<Summary>::release_node_ref(int node_index, int dim)
{
  if (node_index == -1) {
    return 0;
  }
  if (dim == dims.size()) {
    // this is a summaries ref.
    return summaries.release_ref(node_index);
  } else {
    // this is an internal ref
    assert(node_index < dims.at(dim).size());
    NCDimNode &node = dims.at(dim).at(node_index);
    int new_ref_count = dims.at(dim).nodes.release_ref(node_index);

    if (new_ref_count == 0) {
      release_node_ref(node.left, dim);
      release_node_ref(node.right, dim);
      release_node_ref(node.next, dim+1);
      node.left = -1;
      node.right = -1;
      node.next = -1;
    }
    return new_ref_count;
  }
}

// insert_node returns a pair of indices: the first element is the
// index of the newly inserted node at $dim$.  the second element is
// the index where $summary$ was appended to.  note that this is not
// guaranteed to be thesame as $summary$ itself, particularly in the
// case where two different summaries get added to exactly the same
// address (In other words, when a nanocube cannot resolve two
// different data points)
template <typename Summary>
pair<int, int> Nanocube<Summary>::insert_node
(const Summary &summary, const vector<int> &addresses, int current_node_index, int dim, int bit,
 SummaryCache &summary_cache)
{
  debug_out << "Entering index:" << current_node_index
       << " dim:" << dim
       << " depth:" << bit << endl;
  assert(dims.size() == addresses.size());
  if (dim == dims.size()) {
    debug_out << "Will insert summary" << endl;
    // we're done inserting things on the nanocube itself.
    // all we need to do is add the summary to the summary vector

    // for the last dimension, the summary index is both the index of the
    // inserted node, and the summary index itself.
    pair<int, int> k = make_pair(current_node_index, -1);
    auto f = summary_cache.find(k);
    if (current_node_index == -1) {
      if (f == summary_cache.end()) {
        // forced summary insertion
        debug_out << "forced atomic summary insertion" << endl;
        int new_ref = summaries.insert(summary);
        debug_out << "New summary at " << new_ref << endl;
        summary_cache[k] = new_ref;
        cache_miss++;
        return make_pair(new_ref, new_ref);
      } else {
        int summary_ref = f->second;
        cout << "HIT" << endl;
        debug_out << "summary cache hit for fresh insertion at " << summary_ref << endl;
        cache_hit++;
        return make_pair(summary_ref, summary_ref);
      }
    } else {
      debug_out << "Existing summary at " << current_node_index << endl;
      if (f == summary_cache.end()) {
        debug_out << "forced composite summary insertion" << endl;
        const Summary &other_summary = summaries.values.at(current_node_index);
        Summary new_summary = other_summary + summary;
        int new_ref = summaries.insert(new_summary);
        summary_cache[k] = new_ref;
        debug_out << "New summary (from " << current_node_index << ") at " << new_ref << endl;
        return make_pair(new_ref, new_ref);
      } else {
        int summary_ref = f->second;
        debug_out << "summary cache hit for fresh insertion at " << summary_ref << endl;
        return make_pair(summary_ref, summary_ref);
      }
    }
  }
  assert(dim < dims.size());
  
  NCDim &nc_dim = dims.at(dim);
  NCDimNode current_node = get_children(current_node_index, dim);
  int current_address = addresses.at(dim);
  int width = nc_dim.width;
  pair<int, int> recursion_result;
  if (bit == width) {
    debug_out << "At bottom of refinement tree. index:" << current_node_index << " dim:" << dim << " depth:" << bit << endl;
    // we're at the bottom of the refinement tree.
    
    assert(current_node.left == current_node.right && current_node.left == -1);

    // recurse into the next dimension
    debug_out << "recursing into next dimension" << endl;
    recursion_result = insert_node(summary, addresses, current_node.next, dim+1, 0, summary_cache);
    debug_out << "out of next-dim recursion" << endl;

    if (current_node_index == -1) {
      // we're inserting into a new address.
      NCDimNode new_node_at_current_dim { -1, -1, recursion_result.first };
      make_node_ref(recursion_result.first, dim+1);
      int new_index = nc_dim.nodes.insert(new_node_at_current_dim);
      debug_out << "Inserting into a new address (" << new_index << ": -1 -1 " << recursion_result.first << ")" << endl;
      return make_pair(new_index, recursion_result.second);
    } else {
      // we're inserting into an existing address.
      debug_out << "Inserting into an existing address" << endl;
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
    debug_out << "Inside the refinement tree" << endl;
    
    int refinement_node;
    int where_to_insert = get_bit(current_address, width-bit-1);
    debug_out << "bit " << bit << " of " << current_address << " is " << where_to_insert << endl;
    if (where_to_insert) {
      debug_out << "Recursing right" << endl;
      refinement_node = current_node.right;
    } else {
      debug_out << "Recursing left" << endl;
      refinement_node = current_node.left;
    }
    
    // recurse down the refinement tree
    debug_out << "recurse down the refinement tree" << endl;
    recursion_result = insert_node(summary, addresses, refinement_node, dim, bit+1, summary_cache);
    debug_out << "out of ref-tree recursion" << endl;
    NCDimNode &recursion_result_node = nc_dim.at(recursion_result.first);

    // now we do a case analysis on the number of children we had before the insertion.
    if (current_node.left == -1 && current_node.right == -1) {
      debug_out << "we had no children,";
      
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
      debug_out << " inserting (" << new_index << ": " << node_we_will_insert.left << " " << node_we_will_insert.right << " " << node_we_will_insert.next << ")" << endl;
      return make_pair(new_index, recursion_result.second);
    } else if (current_node.left != -1 && current_node.right != -1) {
      debug_out << "node " << current_node_index << " had two children" << endl;
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
        // if (recursion_result.first != current_node.right) {
        //   // need to change references.
        //   release_node_ref(current_node.right, dim);
        //   nc_dim.at(current_node_index).right = recursion_result.first;
        //   make_node_ref(recursion_result.first, dim);
        // }
      } else {
        set_left_node_ref(current_node_index, dim, recursion_result.first);
        // similarly to left child
        // if (recursion_result.first != current_node.left) {
        //   // need to change references.
        //   release_node_ref(current_node.left, dim);
        //   nc_dim.at(current_node_index).left = recursion_result.first;
        //   make_node_ref(recursion_result.first, dim);
        // }
      }

      // now we must update the next pointer as well, and that requires another
      // recursive insertion, because no sharing exists.

      pair<int, int> merge_result = merge(dims.at(dim).at(current_node.left).next,
                                          dims.at(dim).at(current_node.right).next,
                                          dim+1, summary_cache);
      set_next_node_ref(current_node_index, dim, merge_result.first);
      // if (merge_result.first != current_node.next) {
      //   release_node_ref(current_node.next, dim+1);
      //   nc_dim.at(current_node_index).next = merge_result.first;
      //   make_node_ref(merge_result.first, dim+1);
      // }
      return make_pair(current_node_index, merge_result.second);
      
      // pair<int, int> next_recursion_result = insert_node(summary, addresses, current_node.next, dim+1, 0, summary_cache);
      // // now we check for reference updates in the same way as the above checks for
      // // left and right, except that we check the next pointer.
      // if (next_recursion_result.first != current_node.next) {
      //   release_node_ref(current_node.next, dim+1);
      //   nc_dim.at(current_node_index).next = next_recursion_result.first;
      // }
      // return make_pair(current_node_index, next_recursion_result.second);
    } else {
      assert((current_node.left == -1 && current_node.right != -1) ||
             (current_node.left != -1 && current_node.right == -1));
      debug_out << "we had one child " << current_node.left << " " << current_node.right << endl;
      // we had exactly one child.
      
      if (where_to_insert) {
        // insertion happened on the right, need to update right child
        // if recursion_result.first is equal to current_node.right, we
        // don't need to change references, because we're still going to refer
        // to the same node.
        set_right_node_ref(current_node_index, dim, recursion_result.first);
        // if (recursion_result.first != current_node.right) {
        //   // need to change references.
        //   release_node_ref(current_node.right, dim);
        //   nc_dim.at(current_node_index).right = recursion_result.first;
        //   make_node_ref(recursion_result.first, dim);
        // }
      } else {
        // similarly to left child
        set_left_node_ref(current_node_index, dim, recursion_result.first);
        // if (recursion_result.first != current_node.left) {
        //   // need to change references.
        //   release_node_ref(current_node.left, dim);
        //   nc_dim.at(current_node_index).left = recursion_result.first;
        //   make_node_ref(recursion_result.first, dim);
        // }
      }

      debug_out << "Before merge var init" << endl;
      debug_out << "  We want to merge dim:" << dim
                << " left:" << current_node.left
                << " right:" << current_node.right << endl;
      
      // after the insertion, 
      // we need a merge of the nodes.
      // if (nc_dim.at(current_node_index).left != -1 && nc_dim.at(current_node_index).right != -1) {
      // "merging"

      const NCDimNode &current = nc_dim.at(current_node_index);
       NCDimNode new_children = get_children(current_node_index, dim);
      
      int left_next = get_children(current.left, dim).next,
         right_next = get_children(current.right, dim).next;
      debug_out << "Merging left:" << left_next << " right:" << right_next << " dim:" << dim+1 << endl;

      pair<int, int> merge_result = merge(left_next, right_next, dim+1, summary_cache);

      debug_out << "Out of merge" << endl;

      debug_out << "mutating node ("
                << current.left << ","
                << current.right << ","
                << current.next
                << "), setting its next to " << merge_result.first << endl;
      set_next_node_ref(current_node_index, dim, merge_result.first);

      // if (merge_result.first != current_node.next) {
      //   release_node_ref(current_node.next, dim+1);
      //   nc_dim.at(current_node_index).next = merge_result.first;
      //   make_node_ref(merge_result.first, dim+1);
      // }
      return make_pair(current_node_index, merge_result.second);
    }
  }
}

template <typename Summary>
pair<int, int> Nanocube<Summary>::merge
(int left, int right, int dim, SummaryCache &summary_cache)
{
  if (left == -1)
    return make_pair(right, get_summary_index(right, dim));
  if (right == -1)
    return make_pair(left, get_summary_index(left, dim));

  if (dim == dims.size()) {
    auto f = summary_cache.find(make_pair(left, right));
    int new_summary_index;
    if (f == summary_cache.end()) {
      const Summary &left_summary = summaries.values.at(left),
                   &right_summary = summaries.values.at(right);
      Summary new_summary = left_summary + right_summary;
      new_summary_index = summaries.insert(new_summary);
      summary_cache[make_pair(left, right)] = new_summary_index;
      cache_miss++;
    } else {
      new_summary_index = f->second;
      cache_hit++;
    }
    return make_pair(new_summary_index, new_summary_index);
  } else {
    NCDimNode left_node = dims.at(dim).at(left),
             right_node = dims.at(dim).at(right);
    pair<int, int> left_merge = merge(left_node.left,  right_node.left,  dim,   summary_cache),
                  right_merge = merge(left_node.right, right_node.right, dim,   summary_cache),
                   next_merge = merge(left_node.next,  right_node.next,  dim+1, summary_cache);
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
(const Summary &summary, const vector<int> &addresses)
{
  SummaryCache summary_cache;
  // cout << summary_cache.size() << ";";
  // cout << summaries.values.size() << "-";
  cache_hit = cache_miss = 0;
  pair<int, int> result = insert_node(summary, addresses, base_root, 0, 0, summary_cache);
  cout << "hits:" << cache_hit
       << " misses:" << cache_miss
       << " summary_cache:" << summary_cache.size()
       << " summaries:" << summaries.values.size() << endl;
  // cout << summaries.values.size() << " (";
  // cout << summary_cache.size() << ")" << endl;
  make_node_ref(result.first, 0);
  release_node_ref(base_root, 0);
  base_root = result.first;
}

template <typename Summary>
void Nanocube<Summary>::compact() {
  auto previous_map = summaries.compact();
  for (auto rb = dims.rbegin(), re = dims.rend(); re != rb; ++rb) {
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
      previous_map.find(rb->nodes.values[i].right);
      if (f != e) {
        rb->nodes.values[i].right = f->second;
      }
    }
  }
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
    out << endl;
  }
  out << "Summaries" << endl;
  out << "\ti\trc\tvalue" << endl;
  for (int i=0; i<summaries.values.size(); ++i) {
    out << "\t" << i
        << "\t" << summaries.ref_counts.at(i)
         << "\t" << summaries.values.at(i) << endl;
  }
}

/******************************************************************************/

template <typename Summary>
bool Nanocube<Summary>::validate_refcounts()
{
  bool kill = false;
  vector<vector<int> > ref_counts(dims.size()+1);
  for (int i=0; i<ref_counts.size()-1; ++i) {
    NCDim &dim = dims.at(i);
    ref_counts[i] = vector<int>(dim.size(), 0);
  }
  ref_counts.back() = vector<int>(summaries.values.size(), 0);
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
void Nanocube<Summary>::set_left_node_ref(int node_index, int dim, int value)
{
  int to_release = dims.at(dim).at(node_index).left;
  dims.at(dim).at(node_index).left = value;
  make_node_ref(value, dim);
  release_node_ref(to_release, dim);
}

template <typename Summary>
void Nanocube<Summary>::set_right_node_ref(int node_index, int dim, int value)
{
  int to_release = dims.at(dim).at(node_index).right;
  dims.at(dim).at(node_index).right = value;
  make_node_ref(value, dim);
  release_node_ref(to_release, dim);
}

template <typename Summary>
void Nanocube<Summary>::set_next_node_ref(int node_index, int dim, int value)
{
  int to_release = dims.at(dim).at(node_index).next;
  dims.at(dim).at(node_index).next = value;
  make_node_ref(value, dim+1);
  release_node_ref(to_release, dim+1);
}

/******************************************************************************/

/* Local Variables:  */
/* mode: c++         */
/* End:              */
