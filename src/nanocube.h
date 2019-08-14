#ifndef NANOCUBE_H_
#define NANOCUBE_H_

// #define NTRACE

#include <vector>
#include <unordered_map>
#include <algorithm>
#include <boost/assert.hpp>
#include "basecube.h"

namespace nc2 {

template <typename Summary>
struct NanoCube: public BaseCube<Summary>
{
  explicit NanoCube(const std::vector<size_t> &widths):
      BaseCube<Summary>(widths) {}

  NCNodePointerType
  merge_spine(size_t dim, NCNodePointerType cube_node_index, NCNodePointerType spine_node_index)
  {
    TRACE_BLOCK("merge_spine")
    if (dim == this->dims_.size()) {
      // merge summaries
      TRACE_BLOCK("summaries dim, bottomed out");
      TRACE(dim);
      TRACE(cube_node_index);
      TRACE(spine_node_index);
      const Summary &cube_summ = this->summaries_[cube_node_index];
      const Summary &spine_summ = this->summaries_[spine_node_index];
      return this->summaries_.insert(cube_summ + spine_summ);
    }
    BOOST_ASSERT(cube_node_index != -1);
    BOOST_ASSERT(spine_node_index != -1);

    const NCDimNode &cube_node = this->dims_[dim].nodes[cube_node_index];
    const NCDimNode &spine_node = this->dims_[dim].nodes[spine_node_index];

    BOOST_ASSERT(spine_node.left_ == -1 || spine_node.right_ == -1);

    // case 1: bottom of dimension
    if (spine_node.left_ == -1 && spine_node.right_ == -1) {
      TRACE_BLOCK("bottom of dimension");
      BOOST_ASSERT(cube_node.left_ == -1 && cube_node.right_ == -1);
      NCNodePointerType result = merge_spine(dim+1, cube_node.next_, spine_node.next_);
      return this->add_node(dim, -1, -1, result);
    }

    // case 2: cube_node is a singleton path in the same direction of spine_node
    if (cube_node.left_ == -1 && spine_node.left_ == -1 &&
        cube_node.right_ != -1 && spine_node.right_ != -1) {
      // spine points right
      TRACE_BLOCK("singleton path same direction, right");
      NCNodePointerType result = merge_spine(dim, cube_node.right_, spine_node.right_);
      return this->add_node(dim, -1, result, this->dims_[dim].nodes[result].next_);
    }
    if (cube_node.left_ != -1 && spine_node.left_ != -1 &&
        cube_node.right_ == -1 && spine_node.right_ == -1) {
      // spine points left
      TRACE_BLOCK("singleton path same direction, left");
      NCNodePointerType result = merge_spine(dim, cube_node.left_, spine_node.left_);
      return this->add_node(dim, result, -1, this->dims_[dim].nodes[result].next_);
    }

    // case 3: cube_node is a singleton path in the opposite direction of spine_node
    if (cube_node.left_ != -1 && spine_node.left_ == -1 &&
        cube_node.right_ == -1 && spine_node.right_ != -1) {
      // spine points right
      TRACE_BLOCK("singleton path opposite direction, right");
      NCNodePointerType result = merge_spine(dim+1, cube_node.next_, spine_node.next_);
      return this->add_node(dim, cube_node.left_, spine_node.right_, result);
    }
    if (cube_node.left_ == -1 && spine_node.left_ != -1 &&
        cube_node.right_ != -1 && spine_node.right_ == -1) {
      // spine points left
      TRACE_BLOCK("singleton path opposite direction, left");
      NCNodePointerType result = merge_spine(dim+1, cube_node.next_, spine_node.next_);
      return this->add_node(dim, spine_node.left_, cube_node.right_, result);
    }

    // case 4: cube_node is not a singleton path
    if (cube_node.left_ != -1 && spine_node.left_ == -1 &&
        cube_node.right_ != -1 && spine_node.right_ != -1) {
      // spine points right
      TRACE_BLOCK("not singleton path, right");
      NCNodePointerType next = merge_spine(dim+1, cube_node.next_, spine_node.next_);
      NCNodePointerType ref = merge_spine(dim, cube_node.right_, spine_node.right_);
      return this->add_node(dim, cube_node.left_, ref, next);
    }
    if (cube_node.left_ != -1 && spine_node.left_ != -1 &&
        cube_node.right_ != -1 && spine_node.right_ == -1) {
      // spine points left
      TRACE_BLOCK("not singleton path, left");
      NCNodePointerType next = merge_spine(dim+1, cube_node.next_, spine_node.next_);
      NCNodePointerType ref = merge_spine(dim, cube_node.left_, spine_node.left_);
      return this->add_node(dim, ref, cube_node.right_, next);
    }

    TRACE(cube_node);
    TRACE(spine_node);
    BOOST_ASSERT(false);
  }

  // NCNodePointerType add_or_merge(
  //     const Summary &summary, const std::vector<size_t> &addresses,
  //     size_t dim, size_t bit, NCNodePointerType current_node_index)
  // {
  //   if (!is_tainted(dim, current_node_index)) {
  //     add_to(summary, addresses, dim, bit, current_node_index);
  //   } else {
  //   }
  // }

  // add_to mutates the node
  
  // It's safe to mutate a node whenever that node is reachable by a
  // unique path from the root.

  // How do we track this!?

  void
  add_to(const Summary &summary, const std::vector<size_t> &addresses,
         size_t dim, size_t bit, NCNodePointerType current_node_index)
  {
    TRACE_BLOCK("add_to " << dim << " " << bit << " " << current_node_index);
    // reached summaries array: we bottom out here by simply mutating
    // the summary for this particular address.
    if (dim == this->dims_.size()) {
      TRACE_BLOCK("bottom out on summaries");
      BOOST_ASSERT(bit == 0);
      this->summaries_[current_node_index] = this->summaries_[current_node_index] + summary;
      return;
    }
    
    // reached a previously unseen address?
    if (-1 == current_node_index) {
      BOOST_ASSERT(false);
      this->insert_fresh_node(summary, addresses, dim, bit);
      return;
    }

    NCDimNode &current_node = this->dims_[dim].nodes[current_node_index];
    NCNodePointerType l = current_node.left_;
    NCNodePointerType r = current_node.right_;
    NCNodePointerType n = current_node.next_;
    
    size_t width = this->dims_[dim].width;
    bool right = path_refines_towards_right(addresses[dim], bit, width);
    TRACE(addresses[dim]);
    TRACE(current_node);
    TRACE(right);
    TRACE(bit);
    TRACE(dim);
    TRACE(width);

    // WATCH OUT! don't use current_node here because it might have been
    // invalidated by a reallocation of the dim vector in the recursive call
    
    if (l == -1 && r == -1) {
      // bottom of this dimension
      TRACE_BLOCK("bottom");
      add_to(summary, addresses, dim+1, 0, n);
    } else if (right && l == -1 && r != -1) {
      // refine along a singleton path to the right
      TRACE_BLOCK("singleton refine to the right");
      add_to(summary, addresses, dim, bit+1, r);
      this->dims_[dim].nodes[current_node_index].next_ = this->dims_[dim].nodes[r].next_;
      this->report_node(dim, current_node_index);
    } else if (!right && l != -1 && r == -1) {
      // refine along a singleton path to the left
      TRACE_BLOCK("singleton refine to the left");
      add_to(summary, addresses, dim, bit+1, l);
      this->dims_[dim].nodes[current_node_index].next_ = this->dims_[dim].nodes[l].next_;
      this->report_node(dim, current_node_index);
    } else if (right && l != -1 && r == -1) {
      TRACE_BLOCK("create new branching path to the right");
      // create new branching path to the right
      auto new_refine = this->insert_fresh_node(summary, addresses, dim, bit+1);
      TRACE(new_refine);
      this->dims_[dim].nodes[current_node_index].right_ = new_refine;
      this->dims_[dim].nodes[current_node_index].next_ = merge_spine(dim+1, n, this->dims_[dim].nodes[new_refine].next_);
      this->report_node(dim, current_node_index);
    } else if (!right && l == -1 && r != -1) {
      // create new branching path to the left
      TRACE_BLOCK("create new branching path to the left");
      auto new_refine = this->insert_fresh_node(summary, addresses, dim, bit+1);
      TRACE(new_refine);
      this->dims_[dim].nodes[current_node_index].left_ = new_refine;
      this->dims_[dim].nodes[current_node_index].next_ = merge_spine(dim+1, n, this->dims_[dim].nodes[new_refine].next_);
      this->report_node(dim, current_node_index);
    } else {
      // go along one side of an existing path, must add to refinement _and_
      // next, since next points to a non-trivial merged summary.
      BOOST_ASSERT(l != -1 && r != -1);
      if (right) {
        TRACE_BLOCK("add to refine and next on right");
        add_to(summary, addresses, dim, bit+1, r);
        add_to(summary, addresses, dim+1, 0, n);
      } else {
        TRACE_BLOCK("add to refine and next on left");
        add_to(summary, addresses, dim, bit+1, l);
        add_to(summary, addresses, dim+1, 0, n);
      }
    }
  }
  
  void
  insert(const std::vector<size_t> &addresses, const Summary &summary)
  {
    TRACE_BLOCK("insert " << stream_vector<size_t>(addresses));
    // base case: initialize an empty nanocube
    if (this->base_root_ == -1) {
      this->base_root_ = insert_fresh_node(summary, addresses, 0, 0);
      TRACE(this->base_root_);
    } else {
      add_to(summary, addresses, 0, 0, this->base_root_);
    }
  }

};


bool test_naive_cube_and_nanocube_equivalence();
bool test_naive_cube_and_nanocube_equivalence_1();
bool test_naive_cube_and_nanocube_equivalence_2();

};

std::ostream &operator<<(std::ostream &os, const nc2::NCDimNode &node);

#endif
