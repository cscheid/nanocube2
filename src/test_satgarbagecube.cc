#include <iostream>
#include "naivecube.h"
// #include "nanocube.h"
#include "satgarbagecube.h"
#include "tests.h"

int main()
{
  // RUN_TEST(nc2::test_naive_cube_doesnt_smoke);
  // RUN_TEST(nc2::test_naive_cube_and_nanocube_equivalence_1);
  {
    // ENABLE_TRACING;
    RUN_TEST(nc2::test_persistentsat());
    RUN_TEST(nc2::test_persistentsat_lower_bound());
    RUN_TEST(nc2::test_persistentsat_sum());
    RUN_TEST(nc2::test_persistentsat_addition());
    RUN_TEST(nc2::test_naivecube_and_satgarbagecube_equivalence());
  }
}
