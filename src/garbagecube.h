#ifndef GARBAGECUBE_H_
#define GARBAGECUBE_H_

// This is a version of the nanocube data structure that generates a
// lot of garbage during insertion.
//
// When using a complex datatype for summaries (one for which the
// object is not O(1) in space), this is exceptionally inefficient.
//
// Here for comparison and debugging.
//
// The insertion algorithm in this case is particularly simple, so
// this is also good for exposition.

// #define NTRACE

#include <vector>
#include <unordered_map>
#include <algorithm>
#include <boost/assert.hpp>
#include "refcounted_vec.h"
#include "utils.h"
#include "basecube.h"

namespace nc2 {

template <typename Summary>
struct GarbageCube: public BaseCube<Summary>
{
  explicit GarbageCube(const std::vector<size_t> &widths):
      BaseCube<Summary>(widths) {}

  NCNodePointerType merge(
      size_t dim, NCNodePointerType index_1, NCNodePointerType index_2)
  {
    TRACE_BLOCK("merge: " << dim << ", " << index_1 << ", " << index_2);
    if (dim == this->dims_.size()) {
      TRACE_BLOCK("merge: summaries");
      const Summary &summ_1 = this->summaries_[index_1],
                    &summ_2 = this->summaries_[index_2];
      return this->summaries_.insert(summ_1 + summ_2);
    }

    // A merge with an empty node is the same as the original
    // node. This provides one source of sharing in a nanocube.
    if (index_1 == -1) {
      TRACE_BLOCK("merge: left refine empty, return right");
      return index_2;
    }
    if (index_2 == -1) {
      TRACE_BLOCK("merge: right refine empty, return left");
      return index_1;
    }

    const NCDimNode &node_1 = this->dims_[dim].nodes[index_1],
                    &node_2 = this->dims_[dim].nodes[index_2];

    NCNodePointerType l1 = node_1.left_,  l2 = node_2.left_,
                      r1 = node_1.right_, r2 = node_2.right_,
                      n1 = node_1.next_,  n2 = node_2.next_;
    
    // The other source of sharing comes from when the result
    // of a merge is a singleton node. In that case, we can
    // safely share the next edge with the result.
    if (l1 == l2 && r1 == r2 && l1 == -1 && r1 == -1) {
      NCNodePointerType new_next = merge(dim+1, n1, n2);
      const NCDimNode &new_next_node = this->dims_[dim].nodes[new_next];
      return this->add_node(dim, -1, -1, new_next);
    }
    if (l1 == l2 && l1 == -1) {
      TRACE_BLOCK("merge only right");
      NCNodePointerType new_right = merge(dim, r1, r2);
      const NCDimNode &new_right_node = this->dims_[dim].nodes[new_right];
      TRACE(new_right);
      TRACE(new_right_node);
      return this->add_node(dim, -1, new_right, new_right_node.next_);
    }
    if (r1 == r2 && r1 == -1) {
      TRACE_BLOCK("merge only left");
      NCNodePointerType new_left = merge(dim, l1, l2);
      const NCDimNode &new_left_node = this->dims_[dim].nodes[new_left];
      TRACE(new_left);
      TRACE(new_left_node);
      return this->add_node(dim, new_left, -1, new_left_node.next_);
    }

    {
      TRACE_BLOCK("merge: complex, recursing");

      return this->add_node(dim,
                            merge(dim, l1, l2),
                            merge(dim, r1, r2),
                            merge(dim+1, n1, n2));
    }
  }

  void insert(const std::vector<size_t> &addresses, const Summary &summary)
  {
    TRACE_BLOCK("insert " << stream_vector(addresses) << ": " << summary);
    NCNodePointerType spine = this->insert_fresh_node(summary, addresses, 0, 0);
    NCNodePointerType new_root = this->merge(0, spine, this->base_root_);

    this->make_node_ref(new_root, 0);
    this->release_node_ref(this->base_root_, 0);
    this->base_root_ = new_root;

    // at this point, it's likely that the base of spine will have no
    // references pointing to it.  since we won't hold any references,
    // we need to clean it up now.

    if (this->dims_[0].nodes.ref_counts[spine] == 0) {
      TRACE_BLOCK("clean spine");
      this->clean_node(spine, 0);
    }
  }
};

/******************************************************************************/
// tests

bool test_naivecube_and_garbagecube_equivalence();
bool test_garbagecube_1();

};

#endif
