#include <iostream>
// #include "nanocube.h"
#include "sparsesat.h"
#include "tests.h"

int main()
{
  // RUN_TEST(nc2::test_naive_cube_doesnt_smoke);
  // RUN_TEST(nc2::test_naive_cube_and_nanocube_equivalence_1);
  {
    // ENABLE_TRACING;
    RUN_TEST(nc2::test_sparsesat());
    // RUN_TEST(nc2::test_sparsesat_lower_bound());
    RUN_TEST(nc2::test_sparsesat_sum());
    RUN_TEST(nc2::test_sparsesat_addition());

  }
}
