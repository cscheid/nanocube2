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

namespace nc2 {

template <typename T>
struct PersistentSAT
{
  typedef immutable::Array<std::pair<size_t, T> > SAT;
  typedef immutable::ref<SAT> SATRef;

  SATRef sat_;
  
  PersistentSAT(): sat_(SAT::empty()) {}
  PersistentSAT(const PersistentSAT &other): sat_(other.sat_) {}
  explicit PersistentSAT(const SATRef &sat): sat_(sat) {}

  PersistentSAT<T> &operator=(const PersistentSAT<T> &other) {
    if (&other == this)
      return *this;
    sat_ = other.sat_;
    return *this;
  }

  bool operator!=(const PersistentSAT<T> &other) const {
    return !(*this == other);
  }
  bool operator==(const PersistentSAT<T> &other) const{
    return sat_->compare(other.sat_) == 0;
  }
  
  PersistentSAT<T> add(size_t key, const T &val) {
    size_t sz = sat_->size();
    if (sz == 0) {
      return PersistentSAT(sat_->push(std::make_pair(key, val)));
    }
    const std::pair<size_t, T> &last = sat_->last();
    if (last.first < key) {
      return PersistentSAT(sat_->push(std::make_pair(key, last.second + val)));
    }

    // optimize for common case: updating the very last value.
    if (last.first == key) {
      return PersistentSAT(sat_->set(sz-1, std::make_pair(key, last.second + val)));
    }

    // This is the slow path, we have a potentially O(n) operation
    // here!

    size_t split_i = lower_bound(key);
    BOOST_ASSERT(split_i < sat_->size());
    size_t split_key = sat_->get(split_i).first;

    auto left  = sat_->slice(0, split_i);
    auto right = sat_->slice(split_i, sz);

    right = right->modify(
        [&val, &right] (auto t) {
          auto bb = right->begin();
          for (auto b = right->begin(), e = right->end(); b!=e; ++b) {
            size_t i = b.distanceTo(bb);
            t->set(i, std::make_pair((*b).first, (*b).second + val));
          }
        });

    if (split_key == key) {
      return PersistentSAT(left->concat(right));
    } else {
      const T &left_sup = left->last().second;
      return PersistentSAT(left->push(std::make_pair(key, left_sup + val))->concat(right));
    }
    BOOST_ASSERT(false);
  }

  size_t lower_bound(size_t key) {
    if (sat_->size() == 0) {
      return 0;
    }
    size_t left_key  = sat_->first().first, left_i = 0;
    size_t right_key = sat_->last().first, right_i = sat_->size()-1;

    if (key <= left_key) {
      return 0;
    }
    if (key > right_key) {
      return sat_->size();
    }
    if (key == right_key) {
      return right_i;
    }

    size_t center_i = left_i + ((right_i - left_i) >> 1),
         center_key = sat_->get(center_i).first;

    while (right_i - left_i > 1) {
      TRACE(left_i);
      TRACE(right_i);
      TRACE(center_i);
      TRACE(left_key);
      TRACE(right_key);
      TRACE(center_key);
      // invariant: left < index < right
      if (center_key == key)
        return center_i;
      if (center_key < key) {
        left_key = center_key;
        left_i   = center_i;
      } else {
        right_key = center_key;
        right_i   = center_i;
      }
      center_i = left_i + ((right_i - left_i) >> 1);
      center_key = sat_->get(center_i).first;
    }
    TRACE(left_i);
    TRACE(right_i);
    TRACE(center_i);
    TRACE(left_key);
    TRACE(right_key);
    TRACE(center_key);
    if (left_key == key)
      return left_i;
    else
      return right_i;
  }
  
  // T sum(size_t min, size_t max) {
  // }
};

template <typename T>
std::ostream &operator<<(std::ostream &os, const PersistentSAT<T> &sat)
{
  os << "(";
  for (auto &pair: *sat.sat_) {
    os << "(" << pair.first << ", " << pair.second << "), ";
  }
  os << ")";
  return os;
}

/******************************************************************************/
// tests

bool test_persistentsat();
bool test_persistentsat_lower_bound();

}

#endif
