#ifndef UTILS_H_
#define UTILS_H_

#include <iostream>
#include <iterator>
#include <algorithm>
#include "debug.h"

namespace nc2 {

template <typename Summary>
struct CombineSummaryPolicy
{
  Summary total;
  CombineSummaryPolicy(): total() {}
  
  void add(const Summary &v) {
    total += v;
  }
};

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
