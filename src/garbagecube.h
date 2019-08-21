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

  void report_storage()
  {
    for (size_t i=0; i<this->dims_.size(); ++i) {
      std::cerr << "dim " << i << ": " << this->dims_[i].nodes.size() << std::endl; 
      std::cerr << "dim " << i << " free nodes: " << stream_vector(this->dims_[i].nodes.free_list) << std::endl;
   }
    std::cerr << "summaries: " << this->summaries_.size() << std::endl;
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
      this->make_node_ref(spine, 0);
      this->release_node_ref(spine, 0);
      // this->clean_node(spine, 0);
    }
  }
};

/******************************************************************************/
// tests

bool test_naivecube_and_garbagecube_equivalence();
bool test_garbagecube_1();

};

#endif
