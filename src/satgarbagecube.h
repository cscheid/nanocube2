#ifndef SATGARBAGECUBE_H_
#define SATGARBAGECUBE_H_

// SATGarbageCube is a GarbageCube whose last dimension is implemented
// as a sparse Summed Area Table. We use persistent vectors to
// implement the SAT to minimize the effect of copying summaries caused by
// the algorithms in GarbageCube.

#include <immutable/array.h>
#include <utility>
#include <iostream>
#include <boost/assert.hpp>
#include "debug.h"
#include "basecube.h"
#include "persistentsat.h"

namespace nc2 {

/******************************************************************************/

// template <typename T>
// std::vector<T> drop_one(const std::vector<T> &v)
// {
//   std::vector<T> cp(v);
//   cp.pop_back();
//   return cp;
// }

// template <typename Summary>
// struct PersistentSATGarbageCube
// {
//   BaseCube<PersistentSAT<Summary> > base_cube_;
  
//   explicit PersistentSATGarbageCube(const std::vector<size_t> &widths):
//       base_cube_(drop_one(widths)) {}

//   void insert(const std::vector<size_t> &addresses, const Summary &summary) {
//     // FIXME huh, this is annoyingly bad and annoyingly hard to fix nicely
//     std::vector<size_t> first_addresses(drop_one(addresses));

//     PersistentSAT<Summary> singleton;
//     singleton.add_mutate(addresses.back(), summary);

//     NCNodePointerType spine = base_cube_.insert_fresh_node(singleton, first_addresses, 0, 0);
//     NCNodePointerType new_root = base_cube_.merge(0, spine, base_cube_.base_root_);

//     base_cube_.make_node_ref(new_root, 0);
//     base_cube_.release_node_ref(base_cube_.base_root_, 0);
//     base_cube_.base_root_ = new_root;

//     // at this point, it's likely that the base of spine will have no
//     // references pointing to it.  since we won't hold any references,
//     // we need to clean it up now.

//     if (base_cube_.dims_[0].nodes.ref_counts[spine] == 0) {
//       TRACE_BLOCK("clean spine");
//       base_cube_.clean_node(spine, 0);
//     }
//   }

//   template <typename SummaryPolicy>
//   void range_query(
//       SummaryPolicy &policy,
//       const std::vector<std::pair<size_t, size_t> > &bounds) const;

//   void print_dot(std::ostream &os, bool draw_garbage=false) {
//     base_cube_.print_dot(os, draw_garbage);
//   }

//   void dump_nc(bool show_garbage=false) {
//     base_cube_.dump_nc(show_garbage);
//   }
// };

// template <typename Summary, typename SummaryPolicy>
// struct SATSummaryPolicyAdaptor
// {
//   std::pair<size_t, size_t> last_bound_;
//   const PersistentSATGarbageCube<Summary> &sat_cube_;
//   SummaryPolicy &summary_policy_;
  
//   SATSummaryPolicyAdaptor(
//       const PersistentSATGarbageCube<Summary> &sat_cube,
//       std::pair<size_t, size_t> last_bound,
//       SummaryPolicy &summary_policy)
//       : sat_cube_(sat_cube)
//       , last_bound_(last_bound)
//       , summary_policy_(summary_policy) {}

//   void add(const PersistentSAT<Summary> &v) {
//     Summary summ = v.sum(last_bound_.first, last_bound_.second);
//     summary_policy_.add(summ);
//   }
// };

// template <typename Summary>
// template <typename SummaryPolicy>
// void PersistentSATGarbageCube<Summary>::range_query(
//     SummaryPolicy &policy,
//     const std::vector<std::pair<size_t, size_t> > &bounds) const
// {
//   std::vector<std::pair<size_t, size_t> > garbage_bounds = drop_one(bounds);
//   SATSummaryPolicyAdaptor<Summary, SummaryPolicy> adaptor(
//       *this, bounds.back(), policy);
//   base_cube_.range_query(adaptor, garbage_bounds);
// }

// /******************************************************************************/
// // tests
// bool test_naivecube_and_persistentsatgarbagecube_equivalence();

};

#endif
