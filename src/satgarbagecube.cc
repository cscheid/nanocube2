#include "satgarbagecube.h"
#include "tests.h"

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
  CHECK_THAT(s.lower_bound(2) == 0);
  CHECK_THAT(s.lower_bound(3) == 0);
  CHECK_THAT(s.lower_bound(4) == 1);
  CHECK_THAT(s.lower_bound(5) == 1);
  CHECK_THAT(s.lower_bound(6) == 2);
  CHECK_THAT(s.lower_bound(7) == 2);
  CHECK_THAT(s.lower_bound(8) == 3);
  return true;
}

bool test_persistentsat_sum()
{
  {
    PersistentSAT<int> s;
    s = s.add(3, 1).add(5, 1).add(7, 1);

    CHECK_THAT(s.sum(0, 0) == 0);
    CHECK_THAT(s.sum(3, 4) == 1);
    CHECK_THAT(s.sum(4, 5) == 0);
    CHECK_THAT(s.sum(5, 6) == 1);
    CHECK_THAT(s.sum(0, 4) == 1);
    CHECK_THAT(s.sum(0, 6) == 2);
    CHECK_THAT(s.sum(0, 8) == 3);
  }

  {
    PersistentSAT<int> s;
    s = s.add(1, 1).add(11, 1);
    CHECK_THAT(s.sum(5, 13) == 1);
  }
  return true;
}

bool test_persistentsat_addition()
{
  for (size_t i=0; i<100; ++i) {
    TRACE(i);
    PersistentSAT<int> s, s1, s2;
    for (int j=0; j<rand() % 10; ++j) {
      size_t key = rand() % 1024;
      size_t value = rand() % 1024;
      s = s.add(key, value);
      if (rand() % 2 == 1) {
        s1 = s1.add(key, value);
      } else {
        s2 = s2.add(key, value);
      }
    }
    PersistentSAT<int> s3 = s1 + s2;
    TRACE(s);
    TRACE(s1);
    TRACE(s2);
    TRACE(s3);
    CHECK_THAT(s == s3);
  }
  return true;
}

bool test_naivecube_and_satgarbagecube_equivalence()
{
  return test_randomized_equivalence_to_naivecube<SATGarbageCube>(
      {4, 4}, 1000, 2, 10);
}

};
