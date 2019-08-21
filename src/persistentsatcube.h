#ifndef PERSISTENTSATCUBE_H_
#define PERSISTENTSATCUBE_H_

#include <immutable/array.h>
#include <utility>
#include <iostream>
#include <boost/assert.hpp>
#include "persistentsat.h"
#include "debug.h"
#include "basecube.h"

namespace nc2 {

/******************************************************************************/

template <typename T>
std::vector<T> drop_one(const std::vector<T> &v)
{
  std::vector<T> cp(v);
  cp.pop_back();
  return cp;
}


template <typename Summary,
          template <typename> class Cube>
struct PersistentSATCube
{
  Cube<PersistentSAT<Summary> > base_cube_;

  explicit PersistentSATCube(const std::vector<size_t> &widths):
      base_cube_(drop_one(widths)) {}

  void insert(const std::vector<size_t> &addresses, const Summary &summary) {
    PersistentSAT<Summary> singleton;
    singleton.add_mutate(addresses.back(), summary);
    base_cube_.insert(addresses, singleton);
  }

  template <typename SummaryPolicy>
  void range_query(
      SummaryPolicy &policy,
      const std::vector<std::pair<size_t, size_t> > &bounds) const;

  void print_dot(std::ostream &os, bool draw_garbage=false) {
    base_cube_.print_dot(os, draw_garbage);
  }

  void dump_nc(bool show_garbage=false) {
    base_cube_.dump_nc(show_garbage);
  }
  
};

template <typename Summary,
          template <typename> class Cube,
          typename SummaryPolicy>
struct PersistentSATSummaryPolicyAdaptor
{
  std::pair<size_t, size_t> last_bound_;
  const PersistentSATCube<Summary, Cube> &sat_cube_;
  SummaryPolicy &summary_policy_;
  
  PersistentSATSummaryPolicyAdaptor(
      const PersistentSATCube<Summary, Cube> &sat_cube,
      std::pair<size_t, size_t> last_bound,
      SummaryPolicy &summary_policy)
      : sat_cube_(sat_cube)
      , last_bound_(last_bound)
      , summary_policy_(summary_policy) {}

  void add(const PersistentSAT<Summary> &v) {
    Summary summ = v.sum(last_bound_.first, last_bound_.second);
    summary_policy_.add(summ);
  }
};

template <typename Summary,
          template <typename> class Cube>
template <typename SummaryPolicy>
void PersistentSATCube<Summary, Cube>::range_query(
    SummaryPolicy &policy,
    const std::vector<std::pair<size_t, size_t> > &bounds) const
{
  std::vector<std::pair<size_t, size_t> > garbage_bounds = drop_one(bounds);
  PersistentSATSummaryPolicyAdaptor<Summary, Cube, SummaryPolicy> adaptor(
      *this, bounds.back(), policy);
  base_cube_.range_query(adaptor, garbage_bounds);
}

/******************************************************************************/
// tests

bool test_naivecube_and_persistentsatgarbagecube_equivalence();

};

#endif
