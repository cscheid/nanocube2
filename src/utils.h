#ifndef UTILS_H_
#define UTILS_H_

#include <iostream>
#include <iterator>
#include <algorithm>
#include "debug.h"
#include "refcounted_vec.h"

namespace nc2 {

/******************************************************************************/

template <typename Summary>
struct CombineSummaryPolicy
{
  Summary total;
  CombineSummaryPolicy(): total() {}
  
  void add(const Summary &v) {
    total += v;
  }
};

typedef int32_t NCNodePointerType;

/******************************************************************************/
// some structs and functions are shared between GarbageCube,
// NanoCube, etc. They're declared here

struct NCDimNode {
  NCNodePointerType left_, right_, next_;
  NCDimNode(NCNodePointerType left,
            NCNodePointerType right,
            NCNodePointerType next):
      left_(left), right_(right), next_(next) {};
};

struct NCDim {
  RefCountedVec<NCDimNode> nodes;

  size_t width;

  explicit NCDim(size_t w):
      nodes(),
      width(w) {}

  size_t size() const {
    return nodes.size();
  }

  NCDimNode &operator[](NCNodePointerType v) {
    return nodes[size_t(v)];
  }

  const NCDimNode &operator[](NCNodePointerType v) const {
    return nodes[size_t(v)];
  }
};

std::ostream &operator<<(std::ostream &os, const NCDimNode &node);

inline bool get_bit(size_t value, size_t bit) {
  return ((value >> bit) & 1) == 1;
}

inline bool path_refines_towards_right(size_t value, size_t bit, size_t width) {
  return get_bit(value, width - bit - 1);
}

template <typename T>
inline std::vector<T> drop_one(const std::vector<T> &v)
{
  std::vector<T> cp(v);
  cp.pop_back();
  return cp;
}

/******************************************************************************/
// stream_vector: a simple trick to be able to ostream out a vector directly
template <typename T>
struct stream_vector
{
  const std::vector<T> &v_;
  stream_vector(const std::vector<T> &v): v_(v) {}
};

template <typename T>
std::ostream &operator<<(std::ostream &os, const stream_vector<T> &v)
{
  std::copy(v.v_.begin(), v.v_.end(), std::ostream_iterator<T>(os, " "));
  return os;
}

};

#endif
