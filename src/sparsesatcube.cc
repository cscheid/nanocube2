#include "sparsesatcube.h"
#include "tests.h"

// for tests
#include "garbagecube.h"
#include "nanocube.h"

#include <iostream>
#include <utility>

namespace nc2 {

template <typename Summary>
struct SparseSATGarbageCube: public SparseSATCube<Summary, GarbageCube> {
  explicit SparseSATGarbageCube(const std::vector<size_t> &widths):
      SparseSATCube<Summary, GarbageCube>(widths) {}
};

template <typename Summary>
struct SparseSATNanoCube: public SparseSATCube<Summary, NanoCube> {
  explicit SparseSATNanoCube(const std::vector<size_t> &widths):
      SparseSATCube<Summary, NanoCube>(widths) {}
};

bool test_naivecube_and_sparsesatgarbagecube_equivalence()
{
  return test_randomized_equivalence_to_naivecube<SparseSATGarbageCube>(
      {4, 4}, 1000, 2, 10);
}

bool test_naivecube_and_sparsesatnanocube_equivalence()
{
  return test_randomized_equivalence_to_naivecube<SparseSATNanoCube>(
      {4, 4}, 1000, 2, 10);
}

};
