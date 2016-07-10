#include "ref_counted_vec.h"

// assumes a sorted array.
bool sorted_array_has_no_duplicates(const std::vector<int> &v)
{
  if (v.size() <= 1)
    return true;
  auto b = v.begin(), next = b, e = v.end();
  ++next;
  while (next != e) {
    if (*b == *next)
      return false;
    ++b;
    ++next;
  }
  return true;
}