// Copyright 2016 Arizona Board of Regents. See README.md and LICENSE for more info

#include <iostream>
using namespace std;

template <typename T>
int RefCountedVec<T>::make_ref(int index) {
  assert(index < values.size());
  assert(ref_counts.size() == values.size());
  return ++ref_counts[index];
}

template <typename T>
int RefCountedVec<T>::release_ref(int index) {
  assert(index < values.size());
  assert(ref_counts.size() == values.size());
  assert(ref_counts[index] > 0);

  ref_counts[index]--;
  if (ref_counts[index] == 0) {
    free_list.push_back(index);
  }
  return ref_counts[index];
}

template <typename T>
int RefCountedVec<T>::insert(const T &value) {
  int new_ref;
  if (free_list.size() > 0) {
    int free_index = free_list.back();
    free_list.pop_back();
    assert(ref_counts[free_index] == 0);
    values[free_index] = value;
    new_ref = free_index;
  } else {
    values.push_back(value);
    ref_counts.push_back(0);
    new_ref = values.size() - 1;
  }
  return new_ref;
}

template <typename T>
std::map<int, int> RefCountedVec<T>::compact()
{
  std::map<int, int> result;
  bool b = sorted_array_has_no_duplicates(free_list);
  assert(b);
  int values_i = values.size() - 1;
  auto holes_b = free_list.rbegin(), holes_e = free_list.rend();

  // while we still have unpatched holes and we still haven't
  // hit the beginning of the array:
  while (holes_b != holes_e && values_i >= 0) {
    assert(ref_counts[*holes_b] == 0);

    if (*holes_b < values_i) {
      // patch furthest unpatched hole with back of values array.
      values[*holes_b] = values[values_i];
      ref_counts[*holes_b] = ref_counts[values_i];

      // update transposition map of compaction
      result[values_i] = *holes_b;
    }
    assert(*holes_b <= values_i);
    
    ++holes_b;
    --values_i;
    values.pop_back();
    ref_counts.pop_back();
    free_list.pop_back();
  }
  return result;
}

/******************************************************************************/

/* Local Variables:  */
/* mode: c++         */
/* End:              */
