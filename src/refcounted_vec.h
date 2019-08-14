#ifndef REFCOUNTED_VEC_H_
#define REFCOUNTED_VEC_H_

#include <boost/assert.hpp>
#include <vector>
#include <unordered_map>
#include "debug.h"

namespace nc2 {

template <typename T>
struct RefCountedVec
{
  std::vector<T> values;
  std::vector<size_t> ref_counts;
  std::vector<size_t> free_list;

  size_t make_ref(size_t index) {
    BOOST_ASSERT(index < ref_counts.size());
    BOOST_ASSERT(ref_counts.size() == values.size());
    return ++ref_counts[index];
  }

  size_t release_ref(size_t index) {
    BOOST_ASSERT(index < ref_counts.size());
    BOOST_ASSERT(ref_counts.size() == values.size());
    BOOST_ASSERT(ref_counts[index] > 0);
    ref_counts[index]--;
    if (ref_counts[index] == 0) {
      TRACE_BLOCK("Adding " << index << " to free list");
      free_list.push_back(index);
    }
    return ref_counts[index];
  }

  T &operator[](size_t index) { return values[index]; }
  const T&operator[](size_t index) const { return values[index]; }
  T &at(size_t index) { return values.at(index); }
  const T &at(size_t index) const { return values.at(index); }

  inline size_t insert(const T& v) {
    if (free_list.size() > 0) {
      size_t free_index = free_list.back();
      free_list.pop_back();
      values[free_index] = v;
      BOOST_ASSERT(ref_counts[free_index] == 0);
      return free_index;
    } else {
      values.push_back(v);
      ref_counts.push_back(0);
      return values.size() - 1;
    }
  }

  inline size_t size() const {
    return values.size();
  }
    
  std::unordered_map<size_t, size_t> compact() {
    std::unordered_map<size_t, size_t> result;
    if (free_list.size() == 0)
      return result;
    std::sort(free_list.begin(), free_list.end());
    // BOOST_ASSERT(sorted_array_has_no_duplicates(free_list))

    size_t values_i = values.size() - 1;
    size_t holes_b = 0;
    size_t holes_e = free_list.size();
    size_t holes_l = holes_e - 1;

    // while we still have unpatched holes and we haven't yet
    // hit the beginning of the holes array:
    while (holes_b != holes_e) {
      if (ref_counts[values_i] == 0) {
        BOOST_ASSERT(values_i == free_list[holes_l]);
        holes_l -= 1;
        holes_e -= 1;
        values.pop_back();
        ref_counts.pop_back();
        continue;
      }
      BOOST_ASSERT(ref_counts[free_list[holes_b]] == 0);
      BOOST_ASSERT(ref_counts[values_i] != 0);
      BOOST_ASSERT(free_list[holes_b] < values_i);

      // patch furthest unpatched hole with back of values array
      std::swap(values[free_list[holes_b]], values[values_i]);
      std::swap(ref_counts[free_list[holes_b]], ref_counts[values_i]);

      // update transposition map of compaction
      result[values_i] = free_list[holes_b];
      values.pop_back();
      ref_counts.pop_back();

      holes_b += 1;
      if (values_i == 0) {
        break;
      } else {
        values_i -= 1;
      }
    }

    free_list.clear();
    BOOST_ASSERT(free_list.size() == 0);
    return result;
  }

};

};

#endif
