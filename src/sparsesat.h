#ifndef SPARSESAT_H_
#define SPARSESAT_H_

#include "debug.h"
#include <vector>
#include <boost/assert.hpp>

namespace nc2 {

/*
 * A SparseSAT is a sparse version of a 1-D summed-area table.
 */

template <typename T>
inline void add_mutate(
    std::vector<std::pair<size_t, T> > &vec,
    size_t key, const T &val)
{
  auto f = std::lower_bound(
      vec.begin(),
      vec.end(), key,
      [](const std::pair<size_t, T> &e1, size_t k2) {
        return e1.first < k2;
      });
  size_t i = f - vec.begin();
  TRACE(i);
  if (i == vec.size()) {
    // end of the array, simply push back the accumulated value
    if (i == 0) {
      vec.push_back(std::make_pair(key, val));
    } else {
      vec.push_back(std::make_pair(key, vec.back().second + val));
    }
  } else if (vec[i].first == key) {
    // not end of the array, but key already exists, so
    // we just add values and continue
    for (auto e = vec.end(); f<e; ++f) {
      f->second += val;
    }
  } else if (i == 0) {
    vec.insert(f, std::make_pair(key, T()));
    // recreate iterator in case insertion reallocates
    for (auto f = vec.begin() + i, e = vec.end(); f<e; ++f) {
      f->second += val;
    }
  } else {
    vec.insert(f, std::make_pair(key, (f-1)->second));
    // recreate iterator in case insertion reallocates
    for (auto f = vec.begin() + i, e = vec.end(); f<e; ++f) {
      f->second += val;
    }
  }
}

template <typename T>
struct SparseSAT
{
  typedef std::vector<std::pair<size_t, T>> Array;
  Array cum_array_;

  SparseSAT(): cum_array_() {}
  
  explicit SparseSAT(const Array &array): cum_array_(array) {}

  SparseSAT<T> &operator=(const SparseSAT<T> &other) {
    if (&other == this)
      return *this;
    cum_array_ = other.cum_array_;
    return *this;
  }

  bool operator!=(const SparseSAT<T> &other) const {
    return !(*this == other);
  }

  bool operator==(const SparseSAT<T> &other) const {
    TRACE_BLOCK("operator==");
    TRACE(*this);
    TRACE(other);
    bool result = cum_array_ == other.cum_array_;
    TRACE(result);
    return result;
  }

  SparseSAT<T> add(size_t key, const T &val) const {
    TRACE_BLOCK("add " << key << " " << val);
    Array new_array(cum_array_);
    nc2::add_mutate(new_array, key, val);
    return SparseSAT(new_array);
  }

  void add_mutate(size_t key, const T &val) {
    TRACE_BLOCK("add_mutate " << key << " " << val);
    nc2::add_mutate(cum_array_, key, val);
  }

  T sum(size_t min_key, size_t max_key) const {
    size_t min_lb_i = std::lower_bound(
        cum_array_.begin(), cum_array_.end(), min_key,
        [](const std::pair<size_t, T> &e1, size_t k2) {
          return e1.first < k2;
        }) - cum_array_.begin();
    size_t max_lb_i = std::lower_bound(
        cum_array_.begin(), cum_array_.end(), max_key,
        [](const std::pair<size_t, T> &e1, size_t k2) {
          return e1.first < k2;
        }) - cum_array_.begin();
    TRACE(*this);
    TRACE(min_lb_i);
    TRACE(max_lb_i);
    TRACE(min_key);
    TRACE(max_key);
    BOOST_ASSERT(max_lb_i > 0);
    static T zero = T();
    const T &max_v =
        (max_lb_i == cum_array_.size()) ? cum_array_.back().second :
        (max_key <= cum_array_[max_lb_i].first) ? (max_lb_i == 0 ? zero : cum_array_[max_lb_i-1].second) : cum_array_[max_lb_i].second;
    const T &min_v =
        (min_key <= cum_array_[min_lb_i].first && min_lb_i == 0) ? zero :
        (min_key <= cum_array_[min_lb_i].first) ? cum_array_[min_lb_i-1].second : cum_array_[min_lb_i].second;
    TRACE(min_v);
    TRACE(max_v);
    return max_v - min_v;
  }

  SparseSAT<T> &operator+=(const SparseSAT<T> &other) {
    if (other.cum_array_.size() == 0) {
      TRACE_BLOCK("they're empty");
      return *this;
    }
    if (cum_array_.size() == 0) {
      TRACE_BLOCK("we're empty");
      *this = other;
      return *this;
    }

    add_mutate(other.cum_array_[0].first, other.cum_array_[0].second);
    
    for (size_t i=1; i<other.cum_array_.size(); ++i) {
      T val = other.cum_array_[i].second - other.cum_array_[i-1].second;
      add_mutate(other.cum_array_[i].first, val);
    }
    return *this;
  }
};

template <typename T>
SparseSAT<T> operator+(const SparseSAT<T> &v1,
                       const SparseSAT<T> &v2)
{
  SparseSAT<T> result(v1);
  result += v2;
  return result;
}

template <typename T>
std::ostream &operator<<(std::ostream &os, const SparseSAT<T> &sat)
{
  os << "(";
  for (auto &pair: sat.cum_array_) {
    os << "(" << pair.first << ", " << pair.second << "), ";
  }
  os << ")";
  return os;
}

/******************************************************************************/
// tests

bool test_sparsesat();
bool test_sparsesat_lower_bound();
bool test_sparsesat_sum();
bool test_sparsesat_addition();

};

#endif
