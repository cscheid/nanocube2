#pragma once

// Copyright 2016 Arizona Board of Regents. See README.md and LICENSE for more.

#include <vector>
#include <map>
#include <cassert>

// vector of reference-counted values. The reference-counts are "manually"-managed:
// there's currently no RAII support for references, and copies of a reference-counted
// vector copy the reference counts. This is possibly not the correct behavior in all
// instances, but is exactly what we want in the case of copies the refcountedvecs in
// a nanocube.

template <typename T>
struct RefCountedVec {

  // add a reference to an existing value. returns new reference count
  int make_ref(int index);

  // remove a reference from an existing value. returns new reference count
  int release_ref(int index);

  // insert fresh value and return index of reference to it. Returns reference
  // (*not* count, but rather the index)
  int insert(const T &value);

  // compacts the vector, ensuring that free_list.size() == 0 after the call.

  // returns the transposition map of the compaction. It's the responsibility
  // of the caller to update upstream references
  std::map<int, int> compact();
  
  std::vector<T> values;
  std::vector<int> ref_counts;
  std::vector<int> free_list;

  RefCountedVec() {}
  RefCountedVec(const RefCountedVec<T> &other):
      values(other.values),
      ref_counts(other.ref_counts),
      free_list(other.free_list) {}
};

bool sorted_array_has_no_duplicates(const std::vector<int> &v);

#include "ref_counted_vec.inc"
