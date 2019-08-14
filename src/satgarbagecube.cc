#include "satgarbagecube.h"
#include <iostream>
#include <utility>

namespace nc2 {

#define CHECK_AGAINST(d, v)                                              \
if (d != nc2::PersistentSAT<int>(nc2::PersistentSAT<int>::SAT::create(v))) \
    return false;

bool test_persistentsat()
{
  PersistentSAT<int> s;
  s = s.add(0, 3);
  auto l1 = { std::make_pair(0, 3) };
  CHECK_AGAINST(s, l1);

  s = s.add(2, 3);
  auto l2 = { std::make_pair(0, 3), std::make_pair(2, 6) };
  CHECK_AGAINST(s, l2);
  
  s = s.add(2, 3);
  auto l3 = { std::make_pair(0, 3), std::make_pair(2, 9) };
  CHECK_AGAINST(s, l3);

  auto l4 = { std::make_pair(0, 3), std::make_pair(1, 4), std::make_pair(2, 10) };
  CHECK_AGAINST(s.add(1, 1), l4);

  auto l5 = { std::make_pair(0, 3), std::make_pair(1, 5), std::make_pair(2, 11) };
  CHECK_AGAINST(s.add(1, 1).add(1, 1), l5);

  auto l6 = { std::make_pair(0, 4), std::make_pair(2, 10) };
  CHECK_AGAINST(s.add(0, 1), l6);
  
  return true;
}

bool test_persistentsat_lower_bound()
{
  PersistentSAT<int> s;
  s = s.add(3, 1).add(5,1).add(7,1);
  if (s.lower_bound(2) != 0) return false;
  if (s.lower_bound(3) != 0) return false;
  if (s.lower_bound(4) != 1) return false;
  if (s.lower_bound(5) != 1) return false;
  if (s.lower_bound(6) != 2) return false;
  if (s.lower_bound(7) != 2) return false;
  if (s.lower_bound(8) != 3) return false;
  return true;
}

};
