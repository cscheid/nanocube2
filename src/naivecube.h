#ifndef NAIVECUBE_H_
#define NAIVECUBE_H_

#include <vector>
#include <utility>
#include <boost/assert.hpp>
#include "debug.h"

namespace nc2 {

template <typename Summary>
class NaiveCube
{
  std::vector<size_t> widths_;
  std::vector<std::pair<std::vector<size_t>, Summary> > values_;

 public:
  
  explicit NaiveCube(const std::vector<size_t> &widths):
      widths_(widths),
      values_() {}

  NaiveCube(const std::vector<size_t> &widths,
           const std::vector<std::pair<std::vector<size_t>, Summary> > &values):
      widths_(widths),
      values_(values) {}

  
  void insert(const std::vector<size_t> &address, const Summary &summary);

  template <typename SummaryPolicy>
  Summary range_query(
      SummaryPolicy &policy,
      const std::vector<std::pair<size_t, size_t> > &bounds) const;
};

template <typename Summary>
void NaiveCube<Summary>::insert(
    const std::vector<size_t> &address, const Summary &summary)
{
  values_.push_back(std::make_pair(address, summary));
}

template <typename Summary>
template <typename SummaryPolicy>
Summary NaiveCube<Summary>::range_query(
    SummaryPolicy &policy,
    const std::vector<std::pair<size_t, size_t> > &bounds) const
{
  Summary result = Summary();

  BOOST_ASSERT(bounds.size() == widths_.size());

  for (auto &v: values_) {
    bool inside = true;
    BOOST_ASSERT(v.first.size() == bounds.size());
    for (int i=0; i<bounds.size(); ++i) {
      size_t range_b = bounds[i].first,
             range_e = bounds[i].second,
            position = v.first[i];
      TRACE(i);
      TRACE(range_b);
      TRACE(position);
      TRACE(range_e);
      if (position < range_b || position >= range_e) {
        TRACE_BLOCK("outside");
        inside = false;
        break;
      }
    }
    if (inside) {
      TRACE_BLOCK("inside");
      policy.add(v.second);
    }
  }
  return result;
}

bool test_naive_cube_doesnt_smoke();

};

#endif
