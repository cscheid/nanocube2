#include "garbagecube.h"
#include "tests.h"

namespace nc2 {

bool test_naivecube_and_garbagecube_equivalence()
{
  return test_randomized_equivalence_to_naivecube<nc2::GarbageCube>({4, 4, 4, 4}, 1000, 100, 10);
}

bool test_garbagecube_1()
{
  return test_equivalence_to_naivecube<nc2::GarbageCube>(
      {4, 4},
      {{7, 1}, {9, 10}, {2, 8}, {8, 14}, {3, 13}, {8, 5}, {12, 2}, {3, 7}, {7, 1}, {8, 4}},
      { std::make_pair(7, 8), std::make_pair(2, 9) });
}

};
