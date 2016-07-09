#include <algorithm>
#include <iostream>

namespace {
  
inline int get_bit(int value, int bit)
{
  return (value >> bit) & 1;
}
  
// assumes a sorted array.
bool sorted_array_has_no_duplicates(const vector<int> &v)
{
  if (v.size() <= 1)
    return true;
  auto b = v.begin(), next = b, e = v.end();
  ++next;
  while (next != e) {
    if (*b == *next)
      return false;
    ++b;
    ++next;
  }
  return true;
}

};

/******************************************************************************/
// RefCountedVec

template <typename T>
int RefCountedVec<T>::make_ref(int index) {
  assert(index < values.size());
  assert(ref_counts.size() == values.size());
  
  return ++ref_counts[index];
}

template <typename T>
int RefCountedVec<T>::release_ref(int index) {
  assert(index < values.size());
  assert(ref_counts.size() == values.size());
  assert(ref_counts[index] > 0);

  ref_counts[index]--;
  if (ref_counts[index] == 0) {
    free_list.push_back(index);
  }
  return ref_counts[index];
}

template <typename T>
int RefCountedVec<T>::insert(const T &value) {
  int new_ref;
  if (free_list.size() > 0) {
    int free_index = free_list.back();
    free_list.pop_back();
    assert(ref_counts[free_index] == 0);
    values[free_index] = value;
    new_ref = free_index;
  } else {
    values.push_back(value);
    ref_counts.push_back(0);
    new_ref = values.size() - 1;
  }
  make_ref(new_ref);
  return new_ref;
}

template <typename T>
map<int, int> RefCountedVec<T>::compact()
{
  map<int, int> result;
  bool b = sorted_array_has_no_duplicates(free_list);
  assert(b);
  int values_i = values.size() - 1;
  auto holes_b = free_list.rbegin(), holes_e = free_list.rend();

  // while we still have unpatched holes and we still haven't
  // hit the beginning of the array:
  while (holes_b != holes_e && values_i >= 0) {
    assert(ref_counts[*holes_b] == 0);

    if (*holes_b < values_i) {
      // patch furthest unpatched hole with back of values array.
      values[*holes_b] = values[values_i];
      ref_counts[*holes_b] = ref_counts[values_i];

      // update transposition map of compaction
      result[values_i] = *holes_b;
    }
    assert(*holes_b <= values_i);
    
    --holes_b;
    --values_i;
    values.pop_back();
    ref_counts.pop_back();
    free_list.pop_back();
  }
  return result;
}

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
  } while (dim < dims.size());
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
    const NCDimNode &node = dims.at(dim).at(node_index);
    int new_ref_count = dims.at(dim).nodes.release_ref(node_index);

    if (new_ref_count == 0) {
      release_node_ref(node.left, dim);
      release_node_ref(node.right, dim);
      release_node_ref(node.next, dim+1);
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
 map<int, int> &summary_cache)
{
  cout << "Entering index:" << current_node_index
       << " dim:" << dim
       << " depth:" << bit << endl;
  assert(dims.size() == addresses.size());
  if (dim == dims.size()) {
    cout << "Will insert summary" << endl;
    // we're done inserting things on the nanocube itself.
    // all we need to do is add the summary to the summary vector

    // for the last dimension, the summary index is both the index of the
    // inserted node, and the summary index itself.
    if (current_node_index == -1) {
      auto f = summary_cache.find(-1);
      if (f == summary_cache.end()) {
        // forced summary insertion
        cout << "forced atomic summary insertion" << endl;
        int new_ref = summaries.insert(summary);
        cout << "New summary at " << new_ref << endl;
        summary_cache[-1] = new_ref;
        return make_pair(new_ref, new_ref);
      } else {
        int summary_ref = f->second;
        cout << "summary cache hit for fresh insertion at " << summary_ref << endl;
        summaries.make_ref(summary_ref);
        return make_pair(summary_ref, summary_ref);
      }
    } else {
      cout << "Existing summary at " << current_node_index << endl;
      auto f = summary_cache.find(current_node_index);
      if (f == summary_cache.end()) {
        cout << "forced composite summary insertion" << endl;
        const Summary &other_summary = summaries.values.at(current_node_index);
        Summary new_summary = other_summary + summary;
        int new_ref = summaries.insert(new_summary);
        summary_cache[current_node_index] = new_ref;
        cout << "New summary (from " << current_node_index << ") at " << new_ref << endl;
        return make_pair(new_ref, new_ref);
      } else {
        int summary_ref = f->second;
        cout << "summary cache hit for fresh insertion at " << summary_ref << endl;
        summaries.make_ref(summary_ref);
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
    cout << "At bottom of refinement tree. index:" << current_node_index << " dim:" << dim << " depth:" << bit << endl;
    // we're at the bottom of the refinement tree.
    
    assert(current_node.left == current_node.right && current_node.left == -1);

    // recurse into the next dimension
    cout << "recursing into next dimension" << endl;
    recursion_result = insert_node(summary, addresses, current_node.next, dim+1, 0, summary_cache);
    cout << "out of next-dim recursion" << endl;

    if (current_node_index == -1) {
      // we're inserting into a new address.
      cout << "Inserting into a new address" << endl;
      NCDimNode new_node_at_current_dim { -1, -1, recursion_result.first };
      int new_index = nc_dim.nodes.insert(new_node_at_current_dim);
      return make_pair(new_index, recursion_result.second);
    } else {
      // we're inserting into an existing address.
      cout << "Inserting into an existing address" << endl;
      assert(current_node.next != -1);

      // but we're at the bottom of the refinement tree.
      // These two conditions, together, mean that we are simply replacing the next
      // reference at the current node.

      release_node_ref(current_node.next, dim+1);
      nc_dim.at(current_node_index).next = recursion_result.first;
      
      return make_pair(current_node_index, recursion_result.second);
    }
  } else {
    // we're in the interior of the refinement tree
    cout << "Inside the refinement tree" << endl;
    
    int refinement_node;
    int where_to_insert = get_bit(current_address, width-bit-1);
    if (where_to_insert) {
      cout << "Recursing right" << endl;
      refinement_node = current_node.right;
    } else {
      cout << "Recursing left" << endl;
      refinement_node = current_node.left;
    }
    
    // recurse down the refinement tree
    cout << "recurse down the refinement tree" << endl;
    recursion_result = insert_node(summary, addresses, refinement_node, dim, bit+1, summary_cache);
    cout << "out of ref-tree recursion" << endl;
    NCDimNode &recursion_result_node = nc_dim.at(recursion_result.first);

    // now we do a case analysis on the number of children we had before the insertion.
    if (current_node.left == -1 && current_node.right == -1) {
      cout << "we had no children" << endl;
      
      // we had no children, but we were an interior node.
      // this means we're actually a ghost node (because all addresses have the same depth,
      // and so all leaves must be at the bottom of the tree)
      // as a result, we will create a new node now to represent the current address.
      
      // In addition, our next is precisely the next of the recursion
      // result (so sharing occurs).

      // we need to update reference counts;
      if (dim == dims.size() - 1) {
        // If we're the last dimension, then our next actually points to summaries
        summaries.make_ref(recursion_result_node.next);
      } else {
        // otherwise, our next points to the following dims vector.
        dims.at(dim+1).nodes.make_ref(recursion_result_node.next);
      }

      NCDimNode node_we_will_insert(
         (!bit) ? recursion_result.first : -1,
         ( bit) ? recursion_result.first : -1,
         recursion_result_node.next);
      int new_index = nc_dim.nodes.insert(node_we_will_insert);
      
      return make_pair(new_index, recursion_result.second);
    } else if (current_node.left != -1 && current_node.right != -1) {
      cout << "we had two children" << endl;
      // we had two children. 

      // // two children means we need a new summary, because no sharing is possible in this case.
      // // if we inserted on the right, we need to get the summary on the left, and vice-versa
      // int other_child = bit ? current_node.left : current_node.right;
      // int other_summary_index = get_summary_index(other_child, dim);
      // T new_summary = summaries.at(recursion_result.second) + summaries.at(other_summary_index);
      // int new_summary_index = summaries.insert(new_summary);

      if (bit) {
        // insertion happened on the right, need to update right child
        // if recursion_result.first is equal to current_node.right, we
        // don't need to change references, because we're still going to refer
        // to the same node.
        if (recursion_result.first != current_node.right) {
          // need to change references.
          release_node_ref(current_node.right, dim);
          nc_dim.at(current_node_index).right = recursion_result.first;
        }
      } else {
        // similarly to left child
        if (recursion_result.first != current_node.left) {
          // need to change references.
          release_node_ref(current_node.left, dim);
          nc_dim.at(current_node_index).left = recursion_result.first;
        }
      }

      // now we must update the next pointer as well, and that requires another
      // recursive insertion, because no sharing exists.

      pair<int, int> next_recursion_result = insert_node(summary, addresses, current_node.next, dim+1, 0, summary_cache);

      // now we check for reference updates in the same way as the above checks for
      // left and right, except that we check the next pointer.

      if (next_recursion_result.first != current_node.next) {
        release_node_ref(current_node.next, dim+1);
        nc_dim.at(current_node_index).next = next_recursion_result.first;
      }

      return make_pair(current_node_index, next_recursion_result.second);
    } else {
      assert((current_node.left == -1 && current_node.right != -1) ||
             (current_node.left != -1 && current_node.right == -1));
      cout << "we had one child " << current_node.left << " " << current_node.right << endl;
      // we had exactly one child.
      
      if (where_to_insert) {
        // insertion happened on the right, need to update right child
        // if recursion_result.first is equal to current_node.right, we
        // don't need to change references, because we're still going to refer
        // to the same node.
        if (recursion_result.first != current_node.right) {
          // need to change references.
          release_node_ref(current_node.right, dim);
          nc_dim.at(current_node_index).right = recursion_result.first;
        }
      } else {
        // similarly to left child
        if (recursion_result.first != current_node.left) {
          // need to change references.
          release_node_ref(current_node.left, dim);
          nc_dim.at(current_node_index).left = recursion_result.first;
        }
      }

      // if, *after the insertion*, we have two children,
      // then we need a recursive call to next at this depth.
      if (nc_dim.at(current_node_index).left != -1 && nc_dim.at(current_node_index).right != -1) {
        cout << "into internal dimension recursion" << endl;
        pair<int, int> next_recursion_result = insert_node(summary, addresses, current_node.next, dim+1, 0, summary_cache);
        cout << "out of internal dimension recursion" << endl;
        if (next_recursion_result.first != current_node.next) {
          release_node_ref(current_node.next, dim+1);
          nc_dim.at(current_node_index).next = next_recursion_result.first;
        }
        return make_pair(current_node_index, next_recursion_result.second);
      } else {
        // if not, then we still have exactly one child.
        if (recursion_result.first != current_node.next) {
          release_node_ref(current_node.next, dim+1);
          nc_dim.at(current_node_index).next = recursion_result.first;
        }
        return make_pair(current_node_index, recursion_result.second);
      }
    }
  }
}

template <typename Summary>
void Nanocube<Summary>::insert
(const Summary &summary, const vector<int> &addresses)
{
  map<int, int> summary_cache;
  pair<int, int> result = insert_node(summary, addresses, base_root, 0, 0, summary_cache);
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
Nanocube<Summary>::Nanocube(const vector<int> &widths) {
  for (int i=0; i<widths.size(); ++i) {
    NCDim ncd;
    ncd.width = widths[i];
    dims.push_back(ncd);
  }
  base_root = -1;
}

template <typename Summary>
void Nanocube<Summary>::dump_internals()
{
  cout << "Root node:" << base_root << endl;
  for (int i=0; i<dims.size(); ++i) {
    cout << "Dimension " << i << endl;
    cout << "\trc\tleft\tright\tnext" << endl;
    RefCountedVec<NCDimNode> &vec = dims.at(i).nodes;
    for (int j=0; j<vec.values.size(); ++j) {
      cout << "\t" << vec.ref_counts.at(j)
           << "\t" << vec.values.at(j).left
           << "\t" << vec.values.at(j).right
           << "\t" << vec.values.at(j).next << endl;
    }
    cout << endl;
  }
  cout << "Summaries" << endl;
  cout << "\trc\tvalue" << endl;
  for (int i=0; i<summaries.values.size(); ++i) {
    cout << "\t" << summaries.ref_counts.at(i)
         << "\t" << summaries.values.at(i) << endl;
  }
}

/******************************************************************************/

/* Local Variables:  */
/* mode: c++         */
/* End:              */
