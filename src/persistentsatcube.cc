#include "persistentsatcube.h"
#include "tests.h"

// for tests
#include "garbagecube.h"

#include <iostream>
#include <utility>

namespace nc2 {

template <typename Summary>
struct PersistentSATGarbageCube: public PersistentSATCube<Summary, GarbageCube> {
  explicit PersistentSATGarbageCube(const std::vector<size_t> &widths):
      PersistentSATCube<Summary, GarbageCube>(widths) {}
};

bool test_naivecube_and_persistentsatgarbagecube_equivalence()
{
  return test_randomized_equivalence_to_naivecube<PersistentSATGarbageCube>(
      {4, 4}, 1000, 2, 10);
}
};
