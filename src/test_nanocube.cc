#include <iostream>
#include "naivecube.h"
#include "nanocube.h"
#include "tests.h"

int main()
{
  // RUN_TEST(nc2::test_naive_cube_doesnt_smoke);
  // RUN_TEST(nc2::test_naive_cube_and_nanocube_equivalence_1);
  {
    ENABLE_TRACING;
    // RUN_TEST(nc2::test_nanocube_1());
    RUN_TEST(nc2::test_naivecube_and_nanocube_equivalence_3());
  }
}
